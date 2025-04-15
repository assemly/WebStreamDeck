#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "../Managers/TranslationManager.hpp"

// Forward declare UIManager to access updateLocalIP if needed, or pass necessary state/callbacks
// class UIManager;

class UIStatusLogWindow {
public:
    // Constructor now takes TranslationManager
    UIStatusLogWindow(TranslationManager& translationManager);
    ~UIStatusLogWindow() = default;

    // Draw method now takes the necessary state as arguments
    void Draw(bool isServerRunning, int serverPort, const std::string& serverIP, std::function<void()> refreshIpCallback);

private:
    TranslationManager& m_translator;
    // UIManager& m_uiManager; // Removed, using callback instead

     // Language selection state remains here
    int m_currentLangIndex = -1; // Initialize properly
};