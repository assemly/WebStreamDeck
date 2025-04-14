#pragma once

#include <uwebsockets/App.h> // uWebSockets main header
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <thread>
#include <memory>
#include <atomic>
#include <optional> // For optional us_listen_socket_t
#include "ConfigManager.hpp" // Include ConfigManager header

// Use nlohmann/json
using json = nlohmann::json;

// Define an empty struct as user data type for each WebSocket
// to avoid using void, which can cause issues with some compilers/standards.
struct PerSocketData {};

// Define the message handler callback function type
// Parameters: WebSocket connection pointer (with PerSocketData), received JSON object, isBinary flag
using MessageHandler = std::function<void(uWS::WebSocket<false, true, PerSocketData>*, const json&, bool)>;

class CommServer {
public:
    // Modify constructor to accept ConfigManager reference
    explicit CommServer(ConfigManager& configManager);
    ~CommServer();

    // Start the server (synchronous call, but runs the event loop in a separate thread)
    bool start(int port);

    // Stop the server
    void stop();

    // Set the message handler callback
    void set_message_handler(MessageHandler handler);

    // Get the server running state
    bool is_running() const;

private:
    ConfigManager& m_configManager; // Store reference to ConfigManager
    // uWebSockets application (event loop)
    // Needs to be a pointer because App is non-copyable/movable
    // and needs to be created/run within the server thread.
    std::unique_ptr<uWS::App> m_app;
    std::thread m_server_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_should_stop{false};

    // Listening socket (for closing)
    // Optional because it's only valid after successful listening.
    std::optional<us_listen_socket_t*> m_listen_socket;
    // uWS::Loop::defer needs a loop pointer, obtained in constructor.
    uWS::Loop* m_loop = nullptr;

    // Message handler function object
    MessageHandler m_message_handler;

    // Event handling logic setup
    void configure_app(int port);

    // Server run function (executed in the separate thread)
    void run_loop();
}; 