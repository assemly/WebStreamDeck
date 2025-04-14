#include "GifLoader.hpp"
#include <iostream>
#include <vector>
#include <GL/glew.h> // Include GLEW for OpenGL functions

namespace GifLoader {

// Helper function to convert GifColorType to RGBA
void GifColorToRGBA(const GifColorType& gifColor, unsigned char* rgba, unsigned char alpha = 255) {
    rgba[0] = gifColor.Red;
    rgba[1] = gifColor.Green;
    rgba[2] = gifColor.Blue;
    rgba[3] = alpha;
}

bool LoadAnimatedGifFromFile(const char* filename, AnimatedGif& gifData) {
    int error = 0;
    GifFileType* gifFile = DGifOpenFileName(filename, &error);
    if (!gifFile) {
        std::cerr << "Error opening GIF file " << filename << ": " << GifErrorString(error) << std::endl;
        return false;
    }

    // Slurp the GIF into memory
    if (DGifSlurp(gifFile) != GIF_OK) {
        std::cerr << "Error reading GIF file " << filename << ": " << GifErrorString(gifFile->Error) << std::endl;
        DGifCloseFile(gifFile, &error);
        return false;
    }

    if (gifFile->ImageCount <= 0) {
        std::cerr << "Error: GIF file " << filename << " contains no images." << std::endl;
        DGifCloseFile(gifFile, &error);
        return false;
    }

    gifData.width = gifFile->SWidth;
    gifData.height = gifFile->SHeight;
    gifData.frameTextureIds.resize(gifFile->ImageCount);
    gifData.frameDelaysMs.resize(gifFile->ImageCount);

    // Buffer to hold the fully composited frame (RGBA)
    std::vector<unsigned char> canvas(gifData.width * gifData.height * 4, 0);
    // Buffer to hold the *previous* frame's canvas state for disposal methods (optional, simple first)
    // std::vector<unsigned char> previousCanvas(gifData.width * gifData.height * 4, 0);
    int transparentColorIndex = -1; // Track transparency

    for (int i = 0; i < gifFile->ImageCount; ++i) {
        SavedImage* frame = &gifFile->SavedImages[i];
        const GifImageDesc& desc = frame->ImageDesc;
        ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : gifFile->SColorMap;

        if (!colorMap) {
            std::cerr << "Error: GIF frame " << i << " in " << filename << " has no color map." << std::endl;
            // Clean up already created textures before returning
            for(int j = 0; j < i; ++j) glDeleteTextures(1, &gifData.frameTextureIds[j]);
            DGifCloseFile(gifFile, &error);
            return false;
        }

        // --- Get Delay and Transparency --- 
        int delay = 100; // Default delay 100ms (10 * 1/100s)
        transparentColorIndex = -1; // Reset transparency for each frame
        int disposalMode = DISPOSAL_UNSPECIFIED;
        for (int j = 0; j < frame->ExtensionBlockCount; ++j) {
            ExtensionBlock* eb = &frame->ExtensionBlocks[j];
            if (eb->Function == GRAPHICS_EXT_FUNC_CODE) {
                GraphicsControlBlock gcb;
                if (DGifExtensionToGCB(eb->ByteCount, eb->Bytes, &gcb) == GIF_OK) {
                    delay = gcb.DelayTime * 10; // Delay is in 1/100ths of a second
                    if (delay <= 0) delay = 100; // Use default if delay is missing or too short
                    transparentColorIndex = gcb.TransparentColor; // -1 if not transparent
                    disposalMode = gcb.DisposalMode;
                } else {
                    std::cerr << "Warning: Failed to parse GCB for frame " << i << " in " << filename << std::endl;
                }
                break; // Only care about the first GCB
            }
        }
        gifData.frameDelaysMs[i] = delay;

        // --- Simple Compositing (Draw frame onto canvas) ---
        // TODO: Implement proper disposal methods if needed.
        // For now, we just draw the current frame over the previous canvas state.
        // If disposalMode == DISPOSE_BACKGROUND, we should clear the frame's area on the *canvas* first.
        // If disposalMode == DISPOSE_PREVIOUS, we'd need to restore from 'previousCanvas'.

        for (int y = 0; y < desc.Height; ++y) {
            for (int x = 0; x < desc.Width; ++x) {
                int gifIndex = y * desc.Width + x;
                int colorIndex = frame->RasterBits[gifIndex];

                // Check bounds and color index validity
                if (colorIndex < 0 || colorIndex >= colorMap->ColorCount) {
                    // Invalid color index, treat as transparent or skip
                    continue; 
                }

                // Handle transparency
                bool isTransparent = (transparentColorIndex != -1 && colorIndex == transparentColorIndex);
                
                // Calculate position on the full canvas
                int canvasX = desc.Left + x;
                int canvasY = desc.Top + y;

                // Check canvas bounds
                if (canvasX >= 0 && canvasX < gifData.width && canvasY >= 0 && canvasY < gifData.height) {
                    int canvasIndex = (canvasY * gifData.width + canvasX) * 4;
                    const GifColorType& gifColor = colorMap->Colors[colorIndex];
                    GifColorToRGBA(gifColor, &canvas[canvasIndex], isTransparent ? 0 : 255);
                }
            }
        }

        // --- Create OpenGL Texture for the composited frame --- 
        glGenTextures(1, &gifData.frameTextureIds[i]);
        glBindTexture(GL_TEXTURE_2D, gifData.frameTextureIds[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use NEAREST for pixel art often
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure correct byte alignment
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gifData.width, gifData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, canvas.data());
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind

        // Optional: Store current canvas state for DISPOSE_PREVIOUS if implemented later
        // previousCanvas = canvas;
    }

    DGifCloseFile(gifFile, &error);
    gifData.loaded = true;
    gifData.currentFrame = 0;
    gifData.lastFrameTime = 0.0; // Initialize timing

    std::cout << "Successfully loaded GIF: " << filename << " (" << gifFile->ImageCount << " frames)" << std::endl;

    return true;
}

} // namespace GifLoader
