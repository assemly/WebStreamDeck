#include "CommServer.hpp" // Include the header file
#include <iostream>
#include <string_view> // For uWS message payload
#include <chrono>    // For sleep
#include <stdexcept> // For std::runtime_error (though not currently used)

CommServer::CommServer() {
    // Get the main event loop pointer for later defer operations
    m_loop = uWS::Loop::get();
}

CommServer::~CommServer() {
    stop(); // Ensure server is stopped on destruction
}

// Configure the uWebSockets application behavior
void CommServer::configure_app(int port) {
    // Create the App instance within the server thread
    m_app = std::make_unique<uWS::App>();

    // Configure WebSocket behavior for any path ("/*")
    m_app->ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024, // 16MB max payload
        .idleTimeout = 600, // Timeout in seconds (e.g., 10 minutes)

        /* Handlers */
        .open = [](uWS::WebSocket<false, true, PerSocketData> *ws) {
            std::cout << "Client connected. Address: " << ws->getRemoteAddressAsText() << std::endl;
            // Access user data via ws->getUserData() if needed
        },
        .message = [this](uWS::WebSocket<false, true, PerSocketData> *ws, std::string_view message, uWS::OpCode opCode) {
            if (opCode == uWS::OpCode::TEXT) {
                std::cout << "Received message: " << message << std::endl;
                if (m_message_handler) {
                    try {
                        // Parse the message as JSON
                        json payload_json = json::parse(message);
                        // Call the external message handler
                        m_message_handler(ws, payload_json, false);
                    }
                    catch (const json::parse_error& e) {
                        std::cerr << "Failed to parse JSON message: " << e.what() << std::endl;
                        ws->send("{\"error\": \"Invalid JSON format\"}", uWS::OpCode::TEXT);
                    }
                     catch (const std::exception& e) {
                        std::cerr << "Error processing message: " << e.what() << std::endl;
                        ws->send("{\"error\": \"Internal server error\"}", uWS::OpCode::TEXT);
                    }
                }
            } else if (opCode == uWS::OpCode::BINARY) {
                std::cout << "Received binary message of size: " << message.length() << std::endl;
                // Handle binary data if necessary
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
             std::cout << "Client disconnected. Code: " << code << ", Message: " << message << std::endl;
        }
    })
    .listen(port, [this, port](us_listen_socket_t *token) {
        // This callback runs when listening starts (or fails)
        if (token) {
            std::cout << "WebSocket Server listening on port " << port << std::endl;
            m_listen_socket = token; // Store the listen socket
            m_running = true; // Set running state
        } else {
            std::cerr << "Failed to listen on port " << port << std::endl;
            m_running = false;
            m_should_stop = true; // Signal loop to stop if listen failed
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