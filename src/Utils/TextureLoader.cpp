#include "TextureLoader.hpp"
#include <map>
#include <iostream>
#include <vector> // Needed for stb_image

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

    // Texture not in cache, attempt to load
    std::cout << "Loading texture: " << filename << std::endl;
    int width, height, channels;
    // Force 4 channels (RGBA) for consistency with OpenGL formats
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    GLuint textureID = 0; // Default to 0 (failure)

    if (data == nullptr) {
        std::cerr << "Error loading image: " << filename << " - " << stbi_failure_reason() << std::endl;
    } else {
        glGenTextures(1, &textureID);
        if (textureID == 0) {
             std::cerr << "Error: Failed to generate texture ID for " << filename << std::endl;
        } else {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure correct alignment
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glBindTexture(GL_TEXTURE_2D, 0);
            std::cout << "Successfully loaded texture: " << filename << " (ID: " << textureID << ")" << std::endl;
        }
        stbi_image_free(data); // Free the image data once uploaded to OpenGL
    }

    // Cache the result (even if loading failed, textureID will be 0)
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