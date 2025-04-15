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
    std::string expectedPrefix = ASSETS_ICONS_ROOT.string();
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


void WebSocketServer::registerWebSocketHandlers(uWS::App* app) {
     if (!app) return;

    app->ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024, // 16MB max payload
        .idleTimeout = 600, // Timeout in seconds (e.g., 10 minutes)

        /* Handlers */
        .open = [this](uWS::WebSocket<false, true, PerSocketData> *ws) {
            std::cout << "[WS] Client connected. Address: " << ws->getRemoteAddressAsText() << std::endl;
            
            // --- Send initial configuration --- 
            try {
                const auto& buttons = m_configManager.getButtons();
                json layout = json::array();
                for (const auto& btn : buttons) {
                    layout.push_back({
                        {"id", btn.id},
                        {"name", btn.name},
                        {"icon_path", getWebIconPath(btn.icon_path)} // Use helper method
                    });
                }
                json configMsg = {
                    {"type", "initial_config"},
                    {"payload", {{"layout", layout}}}
                };
                ws->send(configMsg.dump(), uWS::OpCode::TEXT);
                std::cout << "[WS] Sent initial config to client." << std::endl; 
            } catch (const std::exception& e) {
                std::cerr << "[WS] Error sending initial config: " << e.what() << std::endl;
            }
            // -------------------------------------
        },
        .message = [this](uWS::WebSocket<false, true, PerSocketData> *ws, std::string_view message, uWS::OpCode opCode) {
            if (opCode == uWS::OpCode::TEXT) {
                std::cout << "[WS] Received message: " << message << std::endl;
                if (m_message_handler) {
                    try {
                        json payload_json = json::parse(message);
                        // Call the external handler, abstracting away the ws pointer
                        m_message_handler(payload_json, false);
                    }
                    catch (const json::parse_error& e) {
                        std::cerr << "[WS] Failed to parse JSON message: " << e.what() << std::endl;
                        ws->send("{\"error\": \"Invalid JSON format\"}", uWS::OpCode::TEXT);
                    }
                     catch (const std::exception& e) {
                        std::cerr << "[WS] Error processing message: " << e.what() << std::endl;
                        ws->send("{\"error\": \"Internal server error\"}", uWS::OpCode::TEXT);
                    }
                }
            } else if (opCode == uWS::OpCode::BINARY) {
                std::cout << "[WS] Received binary message of size: " << message.length() << std::endl;
                // Optionally call handler if it supports binary: m_message_handler(jsonData, true);
            }
        },
        .drain = [](uWS::WebSocket<false, true, PerSocketData> *ws) {
            // Can check ws->getBufferedAmount() and potentially send more data
        },
        .ping = [](uWS::WebSocket<false, true, PerSocketData> *ws, std::string_view) {
            // Pong is sent automatically by uWS
        },
        .pong = [](uWS::WebSocket<false, true, PerSocketData> *ws, std::string_view) {
            // Received pong from client
        },
        .close = [](uWS::WebSocket<false, true, PerSocketData> *ws, int code, std::string_view message) {
             std::cout << "[WS] Client disconnected. Code: " << code << ", Message: " << message << std::endl;
        }
    });
}

// Set the external message handler
void WebSocketServer::set_message_handler(MessageHandler handler) {
    m_message_handler = handler;
} 