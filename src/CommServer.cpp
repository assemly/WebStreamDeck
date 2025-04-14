#include "CommServer.hpp" // Include the header file
#include <iostream>
#include <string_view> // For uWS message payload
#include <chrono>    // For sleep
#include <stdexcept> // For std::runtime_error (though not currently used)
#include <fstream>      // For file reading
#include <sstream>      // For reading file into string
#include <filesystem>   // For path manipulation and checking file existence (C++17)
#include <map>          // For MIME types
#include "ConfigManager.hpp" // Make sure ConfigManager is included

// Define the root directory for web files relative to the executable
// Adjust this path as needed
const std::filesystem::path WEB_ROOT = "web";

// Helper function to determine MIME type from file extension
std::string getMimeType(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    static const std::map<std::string, std::string> mimeTypes = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"}
        // Add more types as needed
    };
    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream"; // Default binary type
}

// Helper function to read file content
std::optional<std::string> readFile(const std::filesystem::path& path) {
    // Security check: Ensure the path is within the web root
    auto canonicalPath = std::filesystem::weakly_canonical(path);
    auto webRootCanonical = std::filesystem::weakly_canonical(WEB_ROOT);
    if (canonicalPath.string().find(webRootCanonical.string()) != 0) {
         std::cerr << "Attempt to access file outside web root: " << path << std::endl;
         return std::nullopt;
     }

    if (!std::filesystem::exists(canonicalPath) || !std::filesystem::is_regular_file(canonicalPath)) {
        return std::nullopt;
    }
    std::ifstream file(canonicalPath, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Constructor now takes ConfigManager reference
CommServer::CommServer(ConfigManager& configManager)
    : m_configManager(configManager) // Initialize reference member
{
    m_loop = uWS::Loop::get();
}

CommServer::~CommServer() {
    stop(); // Ensure server is stopped on destruction
}

// Configure the uWebSockets application behavior (WebSocket AND HTTP)
void CommServer::configure_app(int port) {
    m_app = std::make_unique<uWS::App>();

    // Configure WebSocket behavior
    m_app->ws<PerSocketData>("/*", {
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
                    layout.push_back({{"id", btn.id}, {"name", btn.name}}); // Only send id and name for now
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
                        m_message_handler(ws, payload_json, false);
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
    })
    .listen(port, [this, port](us_listen_socket_t *token) {
        // This callback runs when listening starts (or fails)
        if (token) {
            std::cout << "HTTP/WebSocket Server listening on port " << port << std::endl;
            m_listen_socket = token; // Store the listen socket
            m_running = true; // Set running state
        } else {
            std::cerr << "Failed to listen on port " << port << std::endl;
            m_running = false;
            m_should_stop = true; // Signal loop to stop if listen failed
        }
    });

    // --- HTTP Configuration --- 
    m_app->get("/*", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
        std::string_view url = req->getUrl();
        std::filesystem::path requestedPath = WEB_ROOT;

        if (url == "/") {
            requestedPath /= "index.html"; // Default to index.html for root
        } else {
            // Simple path mapping - remove leading slash if present
            requestedPath /= (url[0] == '/' ? url.substr(1) : url);
        }

        std::cout << "[HTTP] Request for URL: " << url << " mapped to: " << requestedPath << std::endl;

        auto fileContent = readFile(requestedPath);

        if (fileContent) {
            res->writeHeader("Content-Type", getMimeType(requestedPath));
            res->end(*fileContent);
        } else {
            std::cerr << "[HTTP] File not found or access denied: " << requestedPath << std::endl;
            res->writeStatus("404 Not Found");
            res->end("File not found");
        }
    });
}

// Start the server
bool CommServer::start(int port) {
    if (m_running) {
        std::cerr << "Server is already running." << std::endl;
        return false;
    }
    m_should_stop = false; // Reset stop flag

    // Run the event loop in a new thread
    m_server_thread = std::thread([this, port]() {
        // App and event loop must be created and run in the same thread
        configure_app(port);
        if (m_running) { // Only run loop if listening succeeded
           m_app->run(); // This blocks until the App stops
        }
        // Cleanup after run() returns
        m_app.reset();
        m_listen_socket.reset();
        m_running = false;
        std::cout << "Server thread finished." << std::endl;
    });

    // Wait briefly to allow the thread to start and set the running state
    // This is a simple way to get an approximate status, not fully robust.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return m_running; // Return the state after attempting to start
}

// Stop the server
void CommServer::stop() {
    if (!m_running || m_should_stop) {
        return; // Already stopped or stopping
    }
    m_should_stop = true;

    // Ensure operations that interact with the loop happen on the loop's thread
    if (m_loop && m_listen_socket) {
        m_loop->defer([this]() {
            if (m_listen_socket) {
                // Close the listening socket to prevent new connections
                us_listen_socket_close(0, *m_listen_socket);
                m_listen_socket.reset();
                std::cout << "Listen socket closed." << std::endl;
            }
            // Closing the listen socket should eventually cause m_app->run() to return.
            // uWS doesn't have an explicit app->stop().
             std::cout << "Requesting server loop to stop." << std::endl;
            // Optionally, you could try to close all existing connections here,
            // but uWS might handle this when the loop ends.
            // For clean shutdown, iterating ws->close() might be needed.
        });
    }

    // Wait for the server thread to finish
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
    std::cout << "WebSocket Server stopped." << std::endl;
}

// Set the external message handler
void CommServer::set_message_handler(MessageHandler handler) {
    m_message_handler = handler;
}

// Check if the server is running
bool CommServer::is_running() const {
    return m_running;
} 