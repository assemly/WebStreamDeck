#include "TextureLoader.hpp"
#include <map>
#include <iostream>
#include <vector> // Needed for stb_image
#include <cstdio> // For FILE, _wfopen_s, fclose
#include <locale> // For codecvt (alternative conversion method)
#include <codecvt> // For codecvt_utf8
#include "StringUtils.hpp" // For Utf8ToWide

// Define STB_IMAGE_IMPLEMENTATION in *one* CPP file. This is the one.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace TextureLoader {

namespace { // Anonymous namespace for internal linkage
    // Cache for loaded static textures
    std::map<std::string, GLuint> g_staticTextureCache;
} // namespace

GLuint LoadTexture(const std::string& filename) {
    // Check cache first
    auto it = g_staticTextureCache.find(filename);
    if (it != g_staticTextureCache.end()) {
        // Return cached texture ID (even if it's 0, indicating previous load failure)
        // std::cout << "Texture cache hit for: " << filename << " (ID: " << it->second << ")" << std::endl; // Debug logging
        return it->second;
    }

    std::cout << "Loading texture: " << filename << std::endl;
    int width = 0, height = 0, channels = 0;
    unsigned char* data = nullptr;
    GLuint textureID = 0; // Default to 0 (failure)

#ifdef _WIN32
    // Windows: Use wide char path for file opening
    std::wstring wFilename = StringUtils::Utf8ToWide(filename);
    if (wFilename.empty() && !filename.empty()) {
        std::cerr << "[TextureLoader] Failed to convert filename to wstring: " << filename << std::endl;
    } else {
        FILE* fp = nullptr;
        errno_t err = _wfopen_s(&fp, wFilename.c_str(), L"rb");
        if (err == 0 && fp != nullptr) {
            std::cout << "[TextureLoader] Opened file (wide): " << filename << std::endl;
            data = stbi_load_from_file(fp, &width, &height, &channels, 4); // Load from file stream
            fclose(fp);
            if(data == nullptr) {
                std::cerr << "[TextureLoader] Failed to load image data using stbi_load_from_file for: " << filename << " - " << stbi_failure_reason() << std::endl;
            }
        } else {
            std::cerr << "[TextureLoader] Failed to open file (wide) using _wfopen_s. Path: " 
                      << filename << ", Error code: " << err << std::endl;
        }
    }
#else
    // Non-Windows: Assume stbi_load handles UTF-8 paths correctly
    data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if(data == nullptr) {
         std::cerr << "[TextureLoader] Error loading image: " << filename << " - " << stbi_failure_reason() << std::endl;
    }
#endif

    // --- Texture Generation (common path if data is not null) ---
    if (data != nullptr) { 
        glGenTextures(1, &textureID);
        if (textureID == 0) {
             std::cerr << "[TextureLoader] Error: Failed to generate texture ID for " << filename << std::endl;
        } else {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure correct alignment
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glBindTexture(GL_TEXTURE_2D, 0);
            std::cout << "[TextureLoader] Successfully loaded texture: " << filename << " (ID: " << textureID << ")" << std::endl;
        }
        stbi_image_free(data); // Free the image data once uploaded
    }

    // Cache the result (textureID will be 0 on failure)
    g_staticTextureCache[filename] = textureID;
    return textureID;
}

void ReleaseStaticTextures() {
    std::cout << "Releasing " << g_staticTextureCache.size() << " cached static textures..." << std::endl;
    for (auto const& [path, textureId] : g_staticTextureCache) {
        if (textureId != 0) { // Only delete valid texture IDs
            glDeleteTextures(1, &textureId);
             std::cout << "  Deleted static texture: " << path << " (ID: " << textureId << ")" << std::endl;
        }
    }
    g_staticTextureCache.clear(); // Clear the map
}

} // namespace TextureLoader