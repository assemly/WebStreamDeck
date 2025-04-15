#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <map> // Although not directly used, ImGuiFileDialog might need it indirectly? Keep for safety or remove later.
#include "../Managers/ConfigManager.hpp"
#include "../Managers/TranslationManager.hpp"
#include "../Utils/InputUtils.hpp" // For hotkey capture
#include <memory> // For UIManager access

// Forward declare ImGuiFileDialog if only pointers/references are used, or include header
#include <ImGuiFileDialog.h> // Include directly as we use its Instance()

// Forward declarations
class ConfigManager;
class UIManager;

class UIConfigurationWindow {
public:
    UIConfigurationWindow(UIManager& uiManager, ConfigManager& configManager, TranslationManager& translationManager);
    ~UIConfigurationWindow() = default; // Default destructor likely sufficient

    void Draw();
    bool IsVisible() const { return m_isVisible; }
    void Show() { m_isVisible = true; }
    void Hide() { m_isVisible = false; }

private:
    UIManager& m_uiManager;
    ConfigManager& m_configManager;
    TranslationManager& m_translator;

    // State moved from UIManager
    char m_newButtonId[128] = "";
    char m_newButtonName[128] = "";
    int m_newButtonActionTypeIndex = -1;
    char m_newButtonActionParam[256] = "";
    char m_newButtonIconPath[256] = "";
    std::string m_editingButtonId = "";
    bool m_isCapturingHotkey = false;
    bool m_manualHotkeyEntry = false;
    bool m_showDeleteConfirmation = false;
    std::string m_buttonIdToDelete = "";

    // Supported action types, moved from UIManager
    const std::vector<std::string> m_supportedActionTypes = {
        "launch_app",
        "open_url",
        "hotkey",
        "media_volume_up",
        "media_volume_down",
        "media_mute",
        "media_play_pause",
        "media_next_track",
        "media_prev_track",
        "media_stop"
    };

    // Private helper methods if needed (e.g., for file dialog handling) could be added here
    void HandleFileDialog();

    // <<< ADDED: Preset related members >>>
    std::string m_presetsDir = "assets/presetconfig"; // Path to presets
    std::vector<std::string> m_presetFileNames;       // List of found preset names (without .json)
    int m_selectedPresetIndex = -1;                  // Index of selected preset in the combo box/list
    char m_newPresetNameBuffer[128] = "";            // Buffer for "Save As Preset" input

    // <<< ADDED: Helper function to scan the preset directory >>>
    void scanPresetDirectory();

    // Temporary storage for editing layout settings
    int m_tempPageCount;
    int m_tempRowsPerPage;
    int m_tempColsPerPage;

    void loadCurrentSettings(); // Helper to load settings into temp vars

    bool m_isVisible = false;
};