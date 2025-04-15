#pragma once

#include <uwebsockets/App.h> // Needs access to uWS types
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include "../Managers/ConfigManager.hpp" // Needs access to ConfigManager for initial config
#include "../Constants/NetworkConstants.hpp" // <<< UPDATED PATH

// Use nlohmann/json
using json = nlohmann::json;

// Define an empty struct as user data type for each WebSocket
// Could potentially be moved to a common network header if needed elsewhere
struct PerSocketData {}; 

// Define the message handler callback function type
// This defines the *external* handler signature expected by the user of this class
using MessageHandler = std::function<void(const json&, bool)>; 
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

private:
    ConfigManager& m_configManager; // Store reference to ConfigManager
    MessageHandler m_message_handler; // Store the external handler

    // Helper to generate web icon path (extracted from old CommServer::configure_app)
    std::string getWebIconPath(const std::string& configuredPath);
}; 