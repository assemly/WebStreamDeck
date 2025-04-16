// src/UIWindows/Components/PresetManagerComponent.hpp
#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem> // For path operations in scan
#include <iostream>   // For logging
#include <algorithm>  // For transform

// Forward declarations of managers to avoid full includes here
class ConfigManager;
class TranslationManager;
class UIManager; // Needed for notifyLayoutChanged

namespace fs = std::filesystem;

class PresetManagerComponent {
public:
    // Constructor takes references to needed managers
    PresetManagerComponent(ConfigManager& configManager, TranslationManager& translationManager, UIManager& uiManager);

    // Main draw function for this component
    void Draw();

private:
    // References to managers
    ConfigManager& m_configManager;
    TranslationManager& m_translator;
    UIManager& m_uiManager;

    // State for preset management (moved from UIConfigurationWindow)
    std::string m_presetsDir = "assets/presetconfig";
    std::vector<std::string> m_presetFileNames; // Stores UTF-8 names
    int m_selectedPresetIndex = -1;
    char m_newPresetNameBuffer[128] = "";

    // Helper function to scan the preset directory
    void scanPresetDirectory();
};