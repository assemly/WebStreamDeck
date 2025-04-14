#pragma once

#include <vector>
#include <string>
#include <gif_lib.h> // Include giflib header

// Forward declare or include GLuint definition
// Assuming GLEW is included elsewhere (e.g., main.cpp)
// If compilation fails, we might need to include <GL/glew.h> or equivalent here
#ifndef GLuint // Basic guard
    typedef unsigned int GLuint;
#endif

namespace GifLoader {

struct AnimatedGif {
    std::vector<GLuint> frameTextureIds;
    std::vector<int> frameDelaysMs; // Delay in milliseconds for each frame
    int currentFrame = 0;
    double lastFrameTime = 0.0;     // Time the current frame started displaying
    bool loaded = false;
    int width = 0; // Store dimensions
    int height = 0;
};

/**
 * @brief Loads an animated GIF from a file.
 * 
 * Decodes the GIF, creates OpenGL textures for each frame, and stores frame data.
 * 
 * @param filename Path to the GIF file.
 * @param gifData Output structure to be filled with GIF data and textures.
 * @return true if loading was successful, false otherwise.
 */
bool LoadAnimatedGifFromFile(const char* filename, AnimatedGif& gifData);

// Potentially add a function to release textures if needed outside UIManager
// void ReleaseAnimatedGif(AnimatedGif& gifData); 

} // namespace GifLoader