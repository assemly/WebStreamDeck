#pragma once

#include <uwebsockets/App.h> // Needs access to uWS::App, HttpResponse, HttpRequest
#include <filesystem>
#include <string>
#include <optional>
#include "../Constants/NetworkConstants.hpp"

// Define the root directory for web files relative to the executable
// const std::filesystem::path WEB_ROOT = "web";
// const std::filesystem::path ASSETS_ICONS_ROOT = "assets/icons";

class HttpServer {
public:
    HttpServer() = default; // Default constructor is likely sufficient

    // Method to register HTTP route handlers onto an existing uWS::App
    void registerHttpHandlers(uWS::App* app);

private:
    // Helper function to determine MIME type from file extension
    static std::string getMimeType(const std::filesystem::path& path);

    // Helper function to read file content with basic security checks
    static std::optional<std::string> readFile(const std::filesystem::path& path);
}; 