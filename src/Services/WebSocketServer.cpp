#include "WebSocketServer.hpp"
#include <iostream>
#include <string_view>
#include <filesystem>
#include <algorithm> // For std::replace
#include "../Constants/NetworkConstants.hpp"

WebSocketServer::WebSocketServer(ConfigManager& configManager)
    : m_configManager(configManager) {}

// Helper to generate web icon path (extracted from old CommServer::configure_app)
std::string WebSocketServer::getWebIconPath(const std::string& configuredPath) {
    if (configuredPath.empty()) {
        return "";
    }

    std::string webIconPath = "";
    std::filesystem::path fsPath(configuredPath);
    std::string pathStr = configuredPath; // Use a copy for manipulation

    // Normalize separators first
    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

    // Define the expected relative prefix (using constant from NetworkConstants.hpp)
    std::string expectedPrefix = NetworkConstants::ASSETS_ICONS_ROOT.string();
    std::replace(expectedPrefix.begin(), expectedPrefix.end(), '\\', '/');

    // Check if the path starts with the expected relative prefix
    if (pathStr.rfind(expectedPrefix, 0) == 0) {
        // If it starts with "assets/icons", use it directly, ensuring leading slash
        webIconPath = "/" + pathStr;
    } else {
        // Check if it looks like just a filename (no slashes)
        if (pathStr.find('/') == std::string::npos) {
            std::cout << "[WS Icon Path INFO] Path '" << configuredPath
                      << "' looks like a filename. Assuming it's in assets/icons/." << std::endl;
            webIconPath = "/" + expectedPrefix + "/" + pathStr;
        } else {
            // If it's an absolute path or unexpected relative path, log a warning.
            std::cerr << "[WS Icon Path WARN] Path '" << configuredPath
                      << "' is not a standard relative path starting with '" << expectedPrefix
                      << "'. Icon might not load correctly in web UI." << std::endl;
            // Fallback: just prepend a slash, hoping the browser resolves it relative to root
            webIconPath = "/" + pathStr;
        }
    }
    // Ensure no double slashes at the beginning
    if (webIconPath.length() > 1 && webIconPath[0] == '/' && webIconPath[1] == '/') {
        webIconPath = webIconPath.substr(1);
    }
    return webIconPath;
}

// Set the external message handler
void WebSocketServer::set_message_handler(MessageHandler handler) {
    m_message_handler = handler;
}

// Helper to send the initial configuration state to a newly connected client
void WebSocketServer::sendInitialState(uWS::WebSocket<false, true, PerSocketData>* ws) {
    try {
        const auto& buttons = m_configManager.getButtons();
        const auto& layout = m_configManager.getLayoutConfig(); // Get the layout object

        // Construct the final message directly using the layout object
        json initialStateJson;
        initialStateJson["type"] = "initial_state";
        initialStateJson["payload"]["buttons"] = buttons;
        // Directly use the layout object; nlohmann will serialize map to [[key, value]] array
        initialStateJson["payload"]["layout"] = layout; 

        ws->send(initialStateJson.dump(), uWS::OpCode::TEXT);
        std::cout << "[WS Server] Sent initial state to new client (pages as default array)." << std::endl;

    } catch (const json::exception& e) {
        std::cerr << "[WS Server] Error creating initial state JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[WS Server] Error sending initial state: " << e.what() << std::endl;
    }
}

// Handler for new WebSocket connections
void WebSocketServer::on_open(uWS::WebSocket<false, true, PerSocketData>* ws) {
    if (m_is_shutting_down.load()) {
        std::cout << "[WS Server] Rejecting new connection during shutdown." << std::endl;
        ws->end(1001, "Server is shutting down"); // 1001 = Going Away
        return;
    }

    std::cout << "[WS Server] Client connected." << std::endl;
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    m_clients.push_back(ws);
    sendInitialState(ws); // Send current config immediately
}

// Handler for incoming WebSocket messages
void WebSocketServer::on_message(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode) {
    if (m_is_shutting_down.load()) {
        // std::cout << "[WS Server] Ignoring message during shutdown." << std::endl;
        return; // Don't process messages during shutdown
    }

    // std::cout << "[WS Server] Received message: " << message << std::endl;

    if (opCode == uWS::OpCode::TEXT) {
        try {
            json payload = json::parse(message);
            if (m_message_handler) {
                // Pass the parsed JSON and isBinary=false to the external handler
                m_message_handler(payload, false);
            } else {
                 std::cerr << "[WS Server] No message handler set to process message." << std::endl;
            }
        } catch (json::parse_error& e) {
            std::cerr << "[WS Server] Failed to parse incoming JSON message: " << e.what() << std::endl;
            // Optionally send an error back to the client
            // ws->send(json{{"type", "error"}, {"message", "Invalid JSON format"}}.dump(), uWS::OpCode::TEXT);
        } catch (const std::exception& e) {
             std::cerr << "[WS Server] Error processing message: " << e.what() << std::endl;
        }
    } else if (opCode == uWS::OpCode::BINARY) {
         std::cerr << "[WS Server] Received binary message (not currently handled)." << std::endl;
         if (m_message_handler) {
            // If a handler is set, maybe it expects binary? Pass an empty JSON and true.
            // Adjust this if binary messages need specific handling.
            m_message_handler(json{}, true);
         }
    } else {
        // Handle other opcodes like PING, PONG, CLOSE if necessary
    }
}

// Handler for WebSocket disconnections
void WebSocketServer::on_close(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message) {
    std::cout << "[WS Server] Client disconnected. Code: " << code << ", Message: " << message << std::endl;
    // Remove the disconnected client from the list
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), ws), m_clients.end());
}

// Helper function to broadcast a generic message to all connected clients
void WebSocketServer::broadcast(const std::string& message) {
    // std::cout << "[WS Server] Broadcasting generic message: " << message << std::endl;
    for (auto* client : m_clients) {
        client->send(message, uWS::OpCode::TEXT);
    }
}

// Helper function to broadcast the current config state
void WebSocketServer::broadcastCurrentState() {
    try {
        // Prepare the payload exactly like in sendInitialState
        const auto& buttons = m_configManager.getButtons();
        const auto& layout = m_configManager.getLayoutConfig();
        
        json initialStateJson;
        initialStateJson["type"] = "initial_state"; // Use the same message type
        initialStateJson["payload"]["buttons"] = buttons;
        initialStateJson["payload"]["layout"] = layout; // Use default serialization (array for pages)

        std::string stateMessage = initialStateJson.dump();

        std::cout << "[WS Server] Broadcasting current state to " << m_clients.size() << " client(s)." << std::endl;
        broadcast(stateMessage); // Use the generic broadcast helper

    } catch (const json::exception& e) {
        std::cerr << "[WS Server] Error creating state JSON for broadcast: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[WS Server] Error broadcasting state: " << e.what() << std::endl;
    }
}

// Register WebSocket specific handlers with the uWS App
void WebSocketServer::registerWebSocketHandlers(uWS::App* app) {
    if (!app) return;

    // Define WebSocket behavior using the PerSocketData struct (though it's empty now)
    app->ws<PerSocketData>(NetworkConstants::WEBSOCKET_PATH, {
        // Settings
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024, // Example: 16MB
        .idleTimeout = 600, // Example: 10 minutes
        .maxBackpressure = 1 * 1024 * 1024, // Example: 1MB

        // Handlers using member functions via lambda capture
        .open = [this](auto *ws) {
            // Cast ws->getUserData() if needed, but PerSocketData is empty
            this->on_open(ws);
        },
        .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
            this->on_message(ws, message, opCode);
        },
        .drain = [](auto *ws) {
            // Check ws->getBufferedAmount() here
            // std::cout << "[WS Server] Drain event. Buffered amount: " << ws->getBufferedAmount() << std::endl;
        },
        .ping = [](auto *ws, std::string_view message) {
             // std::cout << "[WS Server] Ping received." << std::endl;
        },
        .pong = [](auto *ws, std::string_view message) {
             // std::cout << "[WS Server] Pong received." << std::endl;
        },
        .close = [this](auto *ws, int code, std::string_view message) {
           this->on_close(ws, code, message);
        }
    });

     std::cout << "[WS Server] WebSocket handlers registered for path: " << NetworkConstants::WEBSOCKET_PATH << std::endl;
}

// <<< ADDED: Implementation to close all connections >>>
void WebSocketServer::closeAllConnections() {
    std::cout << "[WS Server] Closing all client connections..." << std::endl;
    std::lock_guard<std::mutex> lock(m_clients_mutex); // Lock the mutex
    // Iterate over a copy of the client pointers to avoid issues while modifying the vector
    std::vector<uWS::WebSocket<false, true, PerSocketData>*> clientsToClose = m_clients;
    
    std::cout << "[WS Server] Found " << clientsToClose.size() << " clients to close." << std::endl;

    for (uWS::WebSocket<false, true, PerSocketData>* ws : clientsToClose) {
        if (ws) { // Check if pointer is valid
             // Use ws->close() or ws->end() depending on uWS version/preference
             // ws->close() sends a close frame and waits for ack (might delay)
             // ws->end() forcefully closes (faster shutdown)
             std::cout << "[WS Server] Forcefully closing connection for client." << std::endl;
             try {
                ws->end(1000, "Server shutting down"); // Use end() for faster shutdown
             } catch (const std::exception& e) {
                  std::cerr << "[WS Server] Exception while closing client connection: " << e.what() << std::endl;
             } // Catch potential errors during close
             
             // We don't remove from m_clients here directly, as the 'close' handler
             // should eventually trigger and handle the removal. 
             // Forcing removal here could lead to double-free or iterator invalidation
             // if the close handler runs concurrently.
             // However, since we are shutting down, perhaps direct removal IS safer?
             // Let's keep it relying on the close handler for now, but 'end' should 
             // prevent the handler from doing much work.
        }
    }
    // Clear the map after attempting to close all. This prevents the close handler
    // from potentially trying to access freed memory if it runs after this function returns
    // but before the server thread fully stops.
    // m_clients.clear(); // Consider if clearing here is safer during shutdown
    std::cout << "[WS Server] Finished closing client connections." << std::endl;
}

// <<< ADDED: Implementation for signalShutdown >>>
void WebSocketServer::signalShutdown() {
    m_is_shutting_down.store(true); // Set the atomic flag
    std::cout << "[WS Server] Shutdown signaled." << std::endl;
} 