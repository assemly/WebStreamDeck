#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <map>
#include <GL/glew.h> // For GLuint
#include "../ConfigManager.hpp"
#include "../ActionExecutor.hpp"
#include "../TranslationManager.hpp"
#include "../Utils/GifLoader.hpp" // For AnimatedGif struct
#include "../Utils/TextureLoader.hpp"

// Forward declare UIManager to avoid circular dependency if needed later
// class UIManager;

class UIButtonGridWindow {
public:
    UIButtonGridWindow(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager);
    ~UIButtonGridWindow(); // Destructor to manage resources if needed

    void Draw();

    // Helper function to load static textures (moved from UIManager)
    // GLuint LoadTextureFromFile(const char* filename);

private:
    ConfigManager& m_configManager;
    ActionExecutor& m_actionExecutor;
    TranslationManager& m_translator;

    // Texture management moved here from UIManager for icons used in this window
    // std::map<std::string, GLuint> m_buttonIconTextures;
    std::map<std::string, GifLoader::AnimatedGif> m_animatedGifTextures;

    void releaseAnimatedGifTextures(); // Helper for GIF textures

    // <<< ADDED: Helper function to draw a single button
    void DrawSingleButton(const ButtonConfig& button, double currentTime, float buttonSize);
};