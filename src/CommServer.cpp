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
const std::filesystem::path WEB_ROOT = "web";
const std::filesystem::path ASSETS_ICONS_ROOT = "assets/icons";

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
    // Security check: Ensure the path is within allowed roots
    // Use weakly_canonical to resolve symlinks etc. before checking
    auto canonicalPath = std::filesystem::weakly_canonical(path);
    auto webRootCanonical = std::filesystem::weakly_canonical(WEB_ROOT);
    auto iconsRootCanonical = std::filesystem::weakly_canonical(ASSETS_ICONS_ROOT);

    // Check if the canonical path starts with either allowed root's canonical path
    bool isInWebRoot = (canonicalPath.string().find(webRootCanonical.string()) == 0);
    bool isInIconsRoot = (canonicalPath.string().find(iconsRootCanonical.string()) == 0);

    if (!isInWebRoot && !isInIconsRoot) {
         std::cerr << "Attempt to access file outside allowed roots: " << path << " (Canonical: " << canonicalPath << ")" << std::endl;
         return std::nullopt;
     }

    // Check if file exists and is a regular file after security check
    if (!std::filesystem::exists(canonicalPath) || !std::filesystem::is_regular_file(canonicalPath)) {
         std::cerr << "File does not exist or is not a regular file: " << canonicalPath << std::endl;
        return std::nullopt;
    }
    
    std::ifstream file(canonicalPath, std::ios::binary);
    if (!file.is_open()) {
         std::cerr << "Could not open file: " << canonicalPath << std::endl;
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
                    std::string webIconPath = ""; // Default to empty string
                    if (!btn.icon_path.empty()) {
                        // --- Convert file path to URL path ---
                        std::filesystem::path fsPath(btn.icon_path);
                        std::string pathStr = btn.icon_path; // Use a copy for manipulation

                        // Normalize separators first
                        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                        // Define the expected relative prefix
                        std::string expectedPrefix = ASSETS_ICONS_ROOT.string();
                        std::replace(expectedPrefix.begin(), expectedPrefix.end(), '\\', '/');

                        // Check if the path starts with the expected relative prefix
                        if (pathStr.rfind(expectedPrefix, 0) == 0) {
                             // If it starts with "assets/icons", use it directly
                             webIconPath = "/" + pathStr;
                        } else {
                             // Check if it looks like just a filename (no slashes)
                             if (pathStr.find('/') == std::string::npos) {
                                  std::cout << "[INFO] Button icon path '" << btn.icon_path 
                                            << "' looks like a filename. Assuming it's in assets/icons/." << std::endl;
                                  webIconPath = "/" + expectedPrefix + "/" + pathStr; 
                             } else {
                                 // If it's an absolute path or unexpected relative path, log a warning.
                                 // Ideally, configuration should store correct relative paths.
                                 std::cerr << "[WARN] Button icon path '" << btn.icon_path 
                                           << "' is not a standard relative path starting with '" << expectedPrefix 
                                           << "'. Icon might not load correctly in web UI." << std::endl;
                                 // As a basic fallback, try using the original path, hoping it might work
                                 // or at least trigger a 404 if server is configured for it.
                                 // A truly robust solution needs consistent relative paths in config.
                                 webIconPath = "/" + pathStr; // Best guess fallback
                             }
                        }
                        // Ensure no double slashes at the beginning (e.g., if pathStr started with /)
                        if (webIconPath.length() > 1 && webIconPath[0] == '/' && webIconPath[1] == '/') {
                            webIconPath = webIconPath.substr(1);
                        }
                        // -----------------------------------------
                    }

                    layout.push_back({
                        {"id", btn.id},
                        {"name", btn.name},
                        {"icon_path", webIconPath} // Send the calculated web path
                    });
                }
                json configMsg = {
                    {"type", "initial_config"},
                    {"payload", {{"layout", layout}}}
                };
                ws->send(configMsg.dump(), uWS::OpCode::TEXT);
                std::cout << "[WS] Sent initial config to client (with processed icon paths)." << std::endl; // Added log
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
                        // Pass the ActionExecutor reference to the handler if needed
                        // Or, preferably, the handler captures it if defined as lambda in main.
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
        std::filesystem::path basePath;
        std::string_view relativeUrl;

        constexpr std::string_view iconsPrefix = "/assets/icons/";
        if (url.rfind(iconsPrefix, 0) == 0) {
            basePath = ASSETS_ICONS_ROOT;
            relativeUrl = url.substr(iconsPrefix.length());
        } else {
            basePath = WEB_ROOT;
            relativeUrl = (url == "/" || url.empty()) ? "index.html" : (url[0] == '/' ? url.substr(1) : url);
            if (relativeUrl.empty() || relativeUrl == "/") {
                 relativeUrl = "index.html";
            }
        }

        std::string relativeUrlStr = std::string(relativeUrl);
        if (relativeUrlStr.find("..") != std::string::npos || 
            (relativeUrlStr.length() > 0 && relativeUrlStr[0] == '.')) 
        {
             std::cerr << "[HTTP] Invalid path requested: " << url << std::endl;
             res->writeStatus("400 Bad Request");
             res->end("Invalid path");
             return;
        }

        std::filesystem::path requestedPath = basePath / relativeUrlStr;

        std::cout << "[HTTP] Request for URL: " << url << " mapped to: " << requestedPath << std::endl;

        auto fileContent = readFile(requestedPath);

        if (fileContent) {
            res->writeHeader("Content-Type", getMimeType(requestedPath));
            res->end(*fileContent);
        } else {
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