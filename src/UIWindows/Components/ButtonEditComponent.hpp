#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "../../Managers/ConfigManager.hpp"
#include "../../Managers/TranslationManager.hpp"
#include "../../Utils/InputUtils.hpp" // For hotkey capture
// Forward declare ImGuiFileDialog if only pointers/references are used, or include header
#include <ImGuiFileDialog.h> // Include directly as we use its Instance()
#include "../Components/ButtonListComponent.hpp" // For PrefilledButtonData

class ButtonEditComponent {
public:
    ButtonEditComponent(ConfigManager& configManager, TranslationManager& translator);

    void Draw();

    // Public method to start editing an existing button
    void StartEdit(const ButtonConfig& buttonConfig);

    // Public method to start adding a new button with prefilled data
    void StartAddNewPrefilled(const PrefilledButtonData& data);

    // Helper to check if the component is currently in edit/add mode
    bool IsEditingOrAdding() const { return !m_editingButtonId.empty() || m_addingNew; }

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
    bool m_addingNew = false;          // Flag specifically for when adding new (vs. editing)
    bool m_isCapturingHotkey = false;
    bool m_manualHotkeyEntry = false;

    // Supported action types (needs to be accessible here)
    // Consider moving this to a shared constant or passing from parent if needed elsewhere
    const std::vector<std::string> m_supportedActionTypes = {
        "launch_app", "open_url", "hotkey", 
        "media_volume_up", "media_volume_down", "media_mute", 
        "media_play_pause", "media_next_track", "media_prev_track", "media_stop",
        // New sound actions based on filenames:
        "play_gong_c3", "play_gong_c4", "play_gong_c5",
        "play_shang_d3", "play_shang_d4", "play_shang_d5",
        "play_jiao_e3", "play_jiao_e4", "play_jiao_e5",
        "play_qingjiao_f3", "play_qingjiao_f4", "play_qingjiao_f5",
        "play_zheng_g3", "play_zheng_g4", "play_zheng_g5", // Assuming zhi = zheng
        "play_yu_a3", "play_yu_a4", "play_yu_a5",
        "play_biangong_b3", "play_biangong_b4", "play_biangong_b5",
        "play_melody_qinghuaci"
        // REMOVED: "play_gong", "play_shang", "play_jiao", "play_zhi", "play_yu", "play_melody_gaoshan"
    };

    // Private helpers
    void ClearForm();
    void HandleFileDialog(); // Handles ImGuiFileDialog results
    void SubmitForm();     // Logic for Add/Save button
};
