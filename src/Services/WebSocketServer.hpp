#pragma once

#include <uwebsockets/App.h> // Needs access to uWS types
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <vector>
#include "../Managers/ConfigManager.hpp" // Needs access to ConfigManager for initial config
#include "../Constants/NetworkConstants.hpp" // <<< UPDATED PATH

// Use nlohmann/json
using json = nlohmann::json;

// Define the actual user data struct for each WebSocket connection
struct PerSocketData {
    // Add any per-connection data here later if needed
    // Example: std::string userId;
};

// Define the message handler callback function type
// This defines the *external* handler signature expected by the user of this class
using MessageHandler = std::function<void(const json&, bool isBinary)>;
// Note: Internal uWS handler still gets the ws pointer, but we abstract it away
//       for the external handler set via set_message_handler.

class WebSocketServer {
public:
    // Constructor requires ConfigManager for initial layout sending
    explicit WebSocketServer(ConfigManager& configManager);

    // Method to register WebSocket handlers onto an existing uWS::App
    void registerWebSocketHandlers(uWS::App* app);

    // Set the external message handler callback
    void set_message_handler(MessageHandler handler);

    // <<< ADDED: Method to broadcast the current state >>>
    void broadcastCurrentState();

private:
    ConfigManager& m_configManager; // Store reference to ConfigManager
    MessageHandler m_message_handler; // Store the external handler
    // Use PerSocketData in the template for the clients vector
    std::vector<uWS::WebSocket<false, true, PerSocketData>*> m_clients; // <<< ADDED: Keep track of connected clients

    // Helper to generate web icon path (extracted from old CommServer::configure_app)
    std::string getWebIconPath(const std::string& configuredPath);

    // <<< ADDED: Helper to send initial state to a client >>>
    void sendInitialState(uWS::WebSocket<false, true, PerSocketData>* ws);

    // <<< ADDED: Private handlers for uWS events >>>
    void on_open(uWS::WebSocket<false, true, PerSocketData>* ws);
    void on_message(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode);
    void on_close(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message);

    // <<< ADDED: Helper to broadcast messages (optional) >>>
    void broadcast(const std::string& message);
}; 