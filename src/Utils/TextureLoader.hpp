#pragma once

#include <string>
#include <GL/glew.h> // For GLuint

namespace TextureLoader {

    // Loads a texture from file, caching results.
    // Returns the OpenGL texture ID, or 0 on failure.
    GLuint LoadTexture(const std::string& filename);

    // Releases all cached static textures. Call this during shutdown.
    void ReleaseStaticTextures();

} // namespace TextureLoader