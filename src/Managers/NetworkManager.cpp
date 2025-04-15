#include "NetworkManager.hpp"
#include <iostream>
#include <chrono> // For sleep


NetworkManager::NetworkManager(ConfigManager& configManager)
    : m_webSocketServer(configManager) // Initialize WebSocketServer with ConfigManager
    // m_httpServer is default initialized
{
    // m_loop = uWS::Loop::get(); // REMOVED FROM HERE
}

NetworkManager::~NetworkManager() {
    stop(); // Ensure server is stopped on destruction
}

// Configure the uWS::App with routes from both servers and start listening
void NetworkManager::configure_and_listen(int port) {
    m_app = std::make_unique<uWS::App>();

    // Register handlers from owned servers
    m_httpServer.registerHttpHandlers(m_app.get());
    m_webSocketServer.registerWebSocketHandlers(m_app.get());

    // Start Listening
    m_app->listen(port, [this, port](us_listen_socket_t *token) {
        if (token) {
            std::cout << "NetworkManager: Successfully listening on port " << port << std::endl;
            m_listen_socket = token;
            m_running = true;
        } else {
            std::cerr << "NetworkManager: Failed to listen on port " << port << std::endl;
            m_running = false;
            m_should_stop = true; // Signal loop to stop if listen failed
        }
    });
}

// Start the network services (HTTP & WebSocket)
bool NetworkManager::start(int port) {
    if (m_running) {
        std::cerr << "NetworkManager: Server is already running." << std::endl;
        return false;
    }
    m_should_stop = false; // Reset stop flag

    // Run the event loop in a new thread
    m_server_thread = std::thread([this, port]() {
        // App and event loop must be created and run in the same thread
        configure_and_listen(port);
        m_loop.store(uWS::Loop::get());
        
        if (m_running) { // Only run loop if listening succeeded
            std::cout << "NetworkManager: Starting uWS event loop..." << std::endl;
            m_app->run(); // This blocks until the App stops
            std::cout << "NetworkManager: uWS event loop finished." << std::endl;
        } else {
             std::cout << "NetworkManager: Skipping uWS event loop due to listen failure." << std::endl;
        }
        // Cleanup after run() returns or is skipped
        m_app.reset();
        m_listen_socket.reset();
        m_running = false;
        std::cout << "NetworkManager: Server thread finished." << std::endl;
    });

    // Basic wait to check if listening started (not fully robust)
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    return m_running;
}

// Stop the network services
void NetworkManager::stop() {
     if (!m_running && !m_server_thread.joinable()) {
        // If not running AND thread is not joinable (already finished/never started)
        return; 
    }
    if (m_should_stop) {
         // Already stopping
         // Wait for the thread if it's still running
         if (m_server_thread.joinable()) {
            std::cout << "NetworkManager: Stop already requested, waiting for thread join..." << std::endl;
            m_server_thread.join();
         }
        return;
    }

    std::cout << "NetworkManager: Initiating stop..." << std::endl;
    m_should_stop = true;

    // <<< ADDED: Signal WebSocket server to stop accepting new connections >>>
    m_webSocketServer.signalShutdown();

    // Force close existing WebSocket connections before stopping the loop
    m_webSocketServer.closeAllConnections();

    // Request the loop to stop its operations via defer
    auto* current_loop = m_loop.load();
    if (current_loop && m_app) { 
        current_loop->defer([this]() {
            if (m_listen_socket) {
                us_listen_socket_close(0, *m_listen_socket);
                m_listen_socket.reset(); 
                std::cout << "NetworkManager: Listen socket closed via defer." << std::endl;
            }
             std::cout << "NetworkManager: Deferred stop actions executed. Loop should exit soon." << std::endl;
            // We don't explicitly stop the loop here; closing the listen socket
            // and potentially existing connections should allow `run()` to return.
        });
    } else {
        std::cout << "NetworkManager: Loop or App not available for deferred stop." << std::endl;
    }

    // Wait for the server thread to complete its execution
    if (m_server_thread.joinable()) {
        std::cout << "NetworkManager: Waiting for server thread to join..." << std::endl;
        m_server_thread.join();
        std::cout << "NetworkManager: Server thread joined." << std::endl;
    } else {
        std::cout << "NetworkManager: Server thread was not joinable upon stop request." << std::endl;
    }
    m_running = false; // Explicitly set state after thread joins
    std::cout << "NetworkManager: Stop sequence complete." << std::endl;
}

// Get the server running state
bool NetworkManager::is_running() const {
    return m_running;
}

// Set the external message handler for WebSocket
void NetworkManager::set_websocket_message_handler(MessageHandler handler) {
    m_webSocketServer.set_message_handler(handler);
}

// <<< ADDED: Implementation for broadcasting WebSocket state >>>
void NetworkManager::broadcastWebSocketState() {
    if (m_running) { // Only broadcast if the server is actually running
        std::cout << "[NetworkManager] Received request to broadcast WebSocket state." << std::endl;
        m_webSocketServer.broadcastCurrentState(); // Delegate to WebSocketServer
    } else {
        std::cout << "[NetworkManager] Server not running, skipping WebSocket broadcast." << std::endl;
    }
} 