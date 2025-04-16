#include "HttpServer.hpp"
#include <fstream>      // For file reading
#include <sstream>      // For reading file into string
#include <map>          // For MIME types
#include <iostream>     // For cerr
#include <iomanip>      // For std::setw, std::setfill, std::hex
#include <string>       // For std::string, std::stoi
#include <cctype>       // For std::isxdigit
#include <vector>       // For MultiByteToWideChar buffer

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>    // Needed for MultiByteToWideChar
#endif

// --- BEGIN ADDED URL DECODE ---
// Helper function to URL-decode a string_view
std::string UrlDecode(const std::string_view& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length()); // Reserve space

    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length() && std::isxdigit(encoded[i+1]) && std::isxdigit(encoded[i+2])) {
            std::string hex = std::string(encoded.substr(i + 1, 2));
            try {
                int value = std::stoi(hex, nullptr, 16);
                decoded += static_cast<char>(value);
                i += 2; 
            } catch (...) { // Catch potential exceptions during stoi
                decoded += '%'; // Append % literally on error
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}
// --- END ADDED URL DECODE ---

// Helper function to determine MIME type from file extension
std::string HttpServer::getMimeType(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    // Ensure lowercase comparison for extensions
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); 
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
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        std::cerr << "[HTTP readFile] Error getting canonical path for " << path << ": " << ec.message() << std::endl;
        return std::nullopt;
    }

    // Use weakly_canonical for roots too, in case they are symlinks etc.
    auto webRootCanonical = std::filesystem::weakly_canonical(NetworkConstants::WEB_ROOT, ec);
     if (ec) { std::cerr << "[HTTP readFile] Error canonicalizing WEB_ROOT " << NetworkConstants::WEB_ROOT << ": " << ec.message() << std::endl; return std::nullopt;}
    auto iconsRootCanonical = std::filesystem::weakly_canonical(NetworkConstants::ASSETS_ICONS_ROOT, ec);
     if (ec) { std::cerr << "[HTTP readFile] Error canonicalizing ICONS_ROOT " << NetworkConstants::ASSETS_ICONS_ROOT << ": " << ec.message() << std::endl; return std::nullopt;}

    // Check if the canonical path starts with either allowed root's canonical path
    std::string canonicalPathStr = canonicalPath.string();
    bool isInWebRoot = !webRootCanonical.empty() && (canonicalPathStr.rfind(webRootCanonical.string(), 0) == 0);
    bool isInIconsRoot = !iconsRootCanonical.empty() && (canonicalPathStr.rfind(iconsRootCanonical.string(), 0) == 0);


    if (!isInWebRoot && !isInIconsRoot) {
         std::cerr << "[HTTP readFile] Attempt to access file outside allowed roots: " << path << " (Canonical: " << canonicalPath << ")" << std::endl;
         return std::nullopt;
     }

    // Check if file exists and is a regular file after security check
    if (!std::filesystem::exists(canonicalPath, ec) || ec || !std::filesystem::is_regular_file(canonicalPath, ec) || ec) {
         std::cerr << "[HTTP readFile] File check failed: " << canonicalPath << " (Error: " << ec.message() << ")" << std::endl;
        return std::nullopt;
    }
    
    // Use filesystem path directly with ifstream (should handle wide paths on Windows)
    std::ifstream file(canonicalPath, std::ios::binary);
    if (!file.is_open()) {
         std::cerr << "[HTTP readFile] Could not open file: " << canonicalPath << std::endl;
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
        res->onAborted([](){
            std::cout << "[HTTP] Request aborted" << std::endl;
        });

        std::string_view url = req->getUrl();
        std::filesystem::path basePath;
        std::string_view relativeUrl;

        constexpr std::string_view iconsPrefix = "/assets/icons/";
        
        if (url.rfind(iconsPrefix, 0) == 0) { 
            basePath = NetworkConstants::ASSETS_ICONS_ROOT;
            relativeUrl = url.substr(iconsPrefix.length());
        } else {
            basePath = NetworkConstants::WEB_ROOT;
            relativeUrl = (url == "/" || url.empty()) ? "index.html" : (url[0] == '/' ? url.substr(1) : url);
             if (relativeUrl.empty()) { 
                 relativeUrl = "index.html";
             }
        }

        // Decode the relative URL path
        std::string decodedRelativeUrlStr = UrlDecode(relativeUrl);

        // Basic path traversal check using the DECODED path
        if (decodedRelativeUrlStr.find("..") != std::string::npos || 
            (decodedRelativeUrlStr.length() > 0 && decodedRelativeUrlStr[0] == '.') ||
            decodedRelativeUrlStr.find(':') != std::string::npos || 
            decodedRelativeUrlStr.find('\\') != std::string::npos) 
        {
             std::cerr << "[HTTP] Invalid path requested (after decode): " << url << " -> " << decodedRelativeUrlStr << std::endl;
             res->writeStatus("400 Bad Request");
             res->end("Invalid path");
             return;
        }

        // Construct path using the DECODED relative path
        std::filesystem::path requestedPath;
        try {
             #ifdef _WIN32
             // Convert decoded UTF-8 string to wstring for path construction on Windows
             int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, decodedRelativeUrlStr.c_str(), -1, NULL, 0);
             if (wideCharSize > 0) {
                 std::vector<wchar_t> wideBuffer(wideCharSize);
                 MultiByteToWideChar(CP_UTF8, 0, decodedRelativeUrlStr.c_str(), -1, wideBuffer.data(), wideCharSize);
                 std::wstring wideDecodedRelativePath(wideBuffer.data());
                  // Construct using wstring version for filesystem path
                  requestedPath = basePath / wideDecodedRelativePath; 
             } else {
                  throw std::runtime_error("MultiByteToWideChar failed to calculate size or convert.");
             }
             #else
             // On non-Windows, direct construction from UTF-8 string is usually fine
             requestedPath = basePath / decodedRelativeUrlStr;
             #endif
        } catch (const std::exception& e) {
             std::cerr << "[HTTP] Error constructing path for '" << decodedRelativeUrlStr << "': " << e.what() << std::endl;
              res->writeStatus("500 Internal Server Error");
              res->end("Error constructing path");
              return;
         }
       
        // Use cached response if available and file hasn't changed? (More advanced)
        
        // For now, always read the file
        std::cout << "[HTTP] Request for URL: " << url << " -> Decoded relative: '" << decodedRelativeUrlStr << "' -> Mapped to: " << requestedPath << std::endl;

        auto fileContent = readFile(requestedPath); // Use static member function

        if (fileContent) {
            res->writeHeader("Content-Type", getMimeType(requestedPath)); // Use static member function
            res->end(*fileContent);
        } else {
            std::cerr << "[HTTP] 404 Not Found for path: " << requestedPath << std::endl;
            res->writeStatus("404 Not Found");
            res->end("File not found");
        }
    });
} 