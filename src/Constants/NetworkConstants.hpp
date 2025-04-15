#pragma once

#include <string>
#include <filesystem> // Include for std::filesystem::path

namespace NetworkConstants {

// Define the path for WebSocket connections
constexpr const char* WEBSOCKET_PATH = "/ws";
// Define shared network-related path constants
const std::filesystem::path WEB_ROOT = "web";
// Define the expected relative root path for icons within the assets directory
// (Used by WebSocketServer to potentially adjust icon paths for web clients)
const std::filesystem::path ASSETS_ICONS_ROOT = "assets/icons";

// Add other network-related constants here if needed, for example:
// constexpr int DEFAULT_HTTP_PORT = 8080;
// constexpr int DEFAULT_WEBSOCKET_PORT = 9002;

} // namespace NetworkConstants