#pragma once

#include <memory> // For std::unique_ptr
#include <thread>
#include <atomic>
#include <optional> // For optional us_listen_socket_t
#include <uwebsockets/App.h> // Forward declaration not enough for unique_ptr
#include "../Services/HttpServer.hpp"
#include "../Services/WebSocketServer.hpp"
#include "../Managers/ConfigManager.hpp" // Needed by WebSocketServer

// Forward declare uWS::Loop to avoid including the full header here if possible
// namespace uWS { struct Loop; } // Actually, unique_ptr needs full definition

class NetworkManager {
public:
    // Constructor takes dependencies needed by owned servers
    NetworkManager(ConfigManager& configManager);
    ~NetworkManager();

    // Start the network services (HTTP & WebSocket)
    bool start(int port);

    // Stop the network services
    void stop();

    // Get the server running state
    bool is_running() const;

    // Set the external message handler for WebSocket
    void set_websocket_message_handler(MessageHandler handler);

private:
    HttpServer m_httpServer; // Owns the HTTP server logic instance
    WebSocketServer m_webSocketServer; // Owns the WebSocket server logic instance

    // uWebSockets application (event loop)
    std::unique_ptr<uWS::App> m_app;
    uWS::Loop* m_loop = nullptr; // Pointer to the event loop
    std::thread m_server_thread; // Thread for running the event loop
    std::atomic<bool> m_running{false}; // Is the server actively listening?
    std::atomic<bool> m_should_stop{false}; // Signal for the server thread to stop

    // Listening socket (for closing)
    std::optional<us_listen_socket_t*> m_listen_socket;

    // Configure the uWS::App with routes from both servers and start listening
    void configure_and_listen(int port);
}; 