#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "../../Managers/ConfigManager.hpp"
#include "../../Managers/TranslationManager.hpp"
#include "../../Utils/InputUtils.hpp" // For hotkey capture
// Forward declare ImGuiFileDialog if only pointers/references are used, or include header
#include <ImGuiFileDialog.h> // Include directly as we use its Instance()

class ButtonEditComponent {
public:
    ButtonEditComponent(ConfigManager& configManager, TranslationManager& translator);

    void Draw();

    // Public method to start editing an existing button
    void StartEdit(const ButtonConfig& buttonConfig);

private:
    ConfigManager& m_configManager;
    TranslationManager& m_translator;

    // Form state (moved from UIConfigurationWindow)
    char m_newButtonId[128] = "";
    char m_newButtonName[128] = "";
    int m_newButtonActionTypeIndex = -1; // Index for the combo box
    char m_newButtonActionParam[256] = "";
    char m_newButtonIconPath[256] = "";
    std::string m_editingButtonId = ""; // If not empty, we are in edit mode
    bool m_isCapturingHotkey = false;
    bool m_manualHotkeyEntry = false;

    // Supported action types (needs to be accessible here)
    // Consider moving this to a shared constant or passing from parent if needed elsewhere
    const std::vector<std::string> m_supportedActionTypes = {
        "launch_app", "open_url", "hotkey", "media_volume_up", "media_volume_down",
        "media_mute", "media_play_pause", "media_next_track", "media_prev_track", "media_stop"
    };

    // Private helpers
    void ClearForm();
    void HandleFileDialog(); // Handles ImGuiFileDialog results
    void SubmitForm();     // Logic for Add/Save button
};
