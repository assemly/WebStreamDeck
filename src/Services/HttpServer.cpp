#include "HttpServer.hpp"
#include <fstream>      // For file reading
#include <sstream>      // For reading file into string
#include <map>          // For MIME types
#include <iostream>     // For cerr

// Helper function to determine MIME type from file extension
std::string HttpServer::getMimeType(const std::filesystem::path& path) {
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

// Helper function to read file content with basic security checks
std::optional<std::string> HttpServer::readFile(const std::filesystem::path& path) {
    // Security check: Ensure the path is within allowed roots
    // Use weakly_canonical to resolve symlinks etc. before checking
    // Note: weakly_canonical might throw if path doesn't exist partially
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        std::cerr << "Error getting canonical path for " << path << ": " << ec.message() << std::endl;
        return std::nullopt;
    }

    auto webRootCanonical = std::filesystem::weakly_canonical(NetworkConstants::WEB_ROOT);
    auto iconsRootCanonical = std::filesystem::weakly_canonical(NetworkConstants::ASSETS_ICONS_ROOT);

    // Check if the canonical path starts with either allowed root's canonical path
    std::string canonicalPathStr = canonicalPath.string();
    bool isInWebRoot = (canonicalPathStr.rfind(webRootCanonical.string(), 0) == 0);
    bool isInIconsRoot = (canonicalPathStr.rfind(iconsRootCanonical.string(), 0) == 0);

    if (!isInWebRoot && !isInIconsRoot) {
         std::cerr << "[HTTP] Attempt to access file outside allowed roots: " << path << " (Canonical: " << canonicalPath << ")" << std::endl;
         return std::nullopt;
     }

    // Check if file exists and is a regular file after security check
    if (!std::filesystem::exists(canonicalPath) || !std::filesystem::is_regular_file(canonicalPath)) {
         std::cerr << "[HTTP] File does not exist or is not a regular file: " << canonicalPath << std::endl;
        return std::nullopt;
    }
    
    std::ifstream file(canonicalPath, std::ios::binary);
    if (!file.is_open()) {
         std::cerr << "[HTTP] Could not open file: " << canonicalPath << std::endl;
        return std::nullopt;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Method to register HTTP route handlers onto an existing uWS::App
void HttpServer::registerHttpHandlers(uWS::App* app) {
    if (!app) return;

    app->get("/*", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
        std::string_view url = req->getUrl();
        std::filesystem::path basePath;
        std::string_view relativeUrl;

        constexpr std::string_view iconsPrefix = "/assets/icons/";
        // Use rfind for prefix check, more standard than find for prefix
        if (url.rfind(iconsPrefix, 0) == 0) { 
            basePath = NetworkConstants::ASSETS_ICONS_ROOT;
            relativeUrl = url.substr(iconsPrefix.length());
        } else {
            basePath = NetworkConstants::WEB_ROOT;
            // Handle root path correctly
            relativeUrl = (url == "/" || url.empty()) ? "index.html" : (url[0] == '/' ? url.substr(1) : url);
             if (relativeUrl.empty()) { // Ensure it's never empty after substr(1)
                 relativeUrl = "index.html";
             }
        }

        // Basic path traversal check
        std::string relativeUrlStr = std::string(relativeUrl);
        if (relativeUrlStr.find("..") != std::string::npos || 
            (relativeUrlStr.length() > 0 && relativeUrlStr[0] == '.')) // Prevent accessing hidden files/dirs
        {
             std::cerr << "[HTTP] Invalid path requested: " << url << std::endl;
             res->writeStatus("400 Bad Request");
             res->end("Invalid path");
             return;
        }

        std::filesystem::path requestedPath = basePath / relativeUrlStr;

        std::cout << "[HTTP] Request for URL: " << url << " mapped to: " << requestedPath << std::endl;

        auto fileContent = readFile(requestedPath); // Use static member function

        if (fileContent) {
            res->writeHeader("Content-Type", getMimeType(requestedPath)); // Use static member function
            res->end(*fileContent);
        } else {
            // Log the failure if readFile didn't already
            // std::cerr << "[HTTP] 404 Not Found for path: " << requestedPath << std::endl;
            res->writeStatus("404 Not Found");
            res->end("File not found");
        }
    });
} 