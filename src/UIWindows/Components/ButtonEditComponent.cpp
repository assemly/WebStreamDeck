// src/UIWindows/Components/ButtonEditComponent.cpp
#include "ButtonEditComponent.hpp"
#include <imgui.h>
#include <iostream> // For logging
#include <cstring>  // For strncpy

// Constructor
ButtonEditComponent::ButtonEditComponent(ConfigManager& configManager, TranslationManager& translator)
    : m_configManager(configManager),
      m_translator(translator)
{}

// Public method called by parent window to initiate editing
void ButtonEditComponent::StartEdit(const ButtonConfig& btnCfg) {
    m_editingButtonId = btnCfg.id; // Mark as editing
    strncpy(m_newButtonId, btnCfg.id.c_str(), sizeof(m_newButtonId) - 1); m_newButtonId[sizeof(m_newButtonId) - 1] = 0;
    strncpy(m_newButtonName, btnCfg.name.c_str(), sizeof(m_newButtonName) - 1); m_newButtonName[sizeof(m_newButtonName) - 1] = 0;
    strncpy(m_newButtonActionParam, btnCfg.action_param.c_str(), sizeof(m_newButtonActionParam) - 1); m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = 0;
    strncpy(m_newButtonIconPath, btnCfg.icon_path.c_str(), sizeof(m_newButtonIconPath) - 1); m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = 0;

    // Find the index for the action type combo box
    m_newButtonActionTypeIndex = -1;
    for(size_t i = 0; i < m_supportedActionTypes.size(); ++i) {
        if (m_supportedActionTypes[i] == btnCfg.action_type) {
            m_newButtonActionTypeIndex = static_cast<int>(i);
            break;
        }
    }
    // Handle case where action type wasn't found (set to 0 or log error?)
    if (m_newButtonActionTypeIndex == -1) {
        std::cerr << "[ButtonEditComponent] Warning: Action type '" << btnCfg.action_type << "' for button ID '" << btnCfg.id << "' not found. Defaulting." << std::endl;
        m_newButtonActionTypeIndex = 0; // Default to the first type
    }

    m_isCapturingHotkey = false; // Ensure capture mode is off when starting edit
    m_manualHotkeyEntry = false; // Reset manual entry flag
    std::cout << "[ButtonEditComponent] Started editing button: " << m_editingButtonId << std::endl;
}

// Helper to clear the form fields and exit edit mode
void ButtonEditComponent::ClearForm() {
    m_newButtonId[0] = '\\0';
    memset(m_newButtonName, 0, sizeof(m_newButtonName));
    m_newButtonActionTypeIndex = -1; // Reset combo box selection
    memset(m_newButtonActionParam, 0, sizeof(m_newButtonActionParam));
    memset(m_newButtonIconPath, 0, sizeof(m_newButtonIconPath));
    std::string cancelledId = m_editingButtonId;
    m_editingButtonId = ""; // Exit edit mode
    m_isCapturingHotkey = false;
    m_manualHotkeyEntry = false;
    if (!cancelledId.empty()) {
        std::cout << "[ButtonEditComponent] Edit cancelled/cleared for button ID: " << cancelledId << std::endl;
    }
}

// Handle results from ImGuiFileDialog
void ButtonEditComponent::HandleFileDialog() {
    std::string selectedFilePath;
    bool updateParam = false;
    bool updateIcon = false;

    ImVec2 minSize = ImVec2(600, 400);
    ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);

    // Dialog for selecting Application File (Action Param)
    // Use keys unique to this component instance if multiple instances could exist
    if (ImGuiFileDialog::Instance()->Display("SelectAppDlgKey_BtnEdit", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            updateParam = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Dialog for selecting Icon File (Icon Path)
    if (ImGuiFileDialog::Instance()->Display("SelectIconDlgKey_BtnEdit", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            updateIcon = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Update buffers
    if (updateParam) {
        strncpy(m_newButtonActionParam, selectedFilePath.c_str(), sizeof(m_newButtonActionParam) - 1);
        m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = '\\0';
    }
    if (updateIcon) {
        strncpy(m_newButtonIconPath, selectedFilePath.c_str(), sizeof(m_newButtonIconPath) - 1);
        m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\\0';
    }
}

// Handle Add/Save button logic
void ButtonEditComponent::SubmitForm() {
    ButtonConfig buttonData;
    buttonData.name = m_newButtonName;
    if (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) {
        buttonData.action_type = m_supportedActionTypes[m_newButtonActionTypeIndex];
    } else {
        buttonData.action_type = ""; // Should not happen
        std::cerr << "[ButtonEditComponent] Error: Invalid action type index during submit." << std::endl;
    }
    buttonData.action_param = m_newButtonActionParam;
    if (buttonData.action_type.rfind("media_", 0) == 0) { // Clear param for media actions
        buttonData.action_param = "";
    }
    buttonData.icon_path = m_newButtonIconPath;

    bool configChanged = false;
    bool saveSuccess = false;
    bool isEditing = !m_editingButtonId.empty();

    if (isEditing) {
        buttonData.id = m_editingButtonId;
        if (buttonData.name.empty()) {
            std::cerr << "[ButtonEditComponent] Error: Updated button name cannot be empty." << std::endl;
        } else if (m_configManager.updateButton(m_editingButtonId, buttonData)) {
            std::cout << "[ButtonEditComponent] Button updated successfully: " << m_editingButtonId << std::endl;
            configChanged = true;
        } else {
            std::cerr << "[ButtonEditComponent] Error: Failed to update button " << m_editingButtonId << "." << std::endl;
        }
    } else { // Adding new button
        buttonData.id = m_newButtonId;
        if (buttonData.id.empty() || buttonData.name.empty()) {
            std::cerr << "[ButtonEditComponent] Error: Cannot add button with empty ID or Name." << std::endl;
        } else if (m_configManager.addButton(buttonData)) {
            std::cout << "[ButtonEditComponent] " << m_translator.get("button_added_success_log") << buttonData.id << std::endl;
            configChanged = true;
        } else {
            std::cerr << "[ButtonEditComponent] " << m_translator.get("add_button_fail_log") << " ID: " << buttonData.id << std::endl;
        }
    }

    // Attempt to save config if changes were made by add/update
    if (configChanged) {
        if (m_configManager.saveConfig()) {
            std::cout << "[ButtonEditComponent] Configuration saved successfully." << std::endl;
            saveSuccess = true;
        } else {
            std::cerr << "[ButtonEditComponent] Error: Failed to save configuration after changes." << std::endl;
        }
    }

    // If add/update and save were successful, clear the form
    if (configChanged && saveSuccess) {
        ClearForm(); // Also exits edit mode if was editing
    }
    // If only configChanged was true but save failed, the form retains data for user to retry? Or clear? Currently retains.
}


// Main Draw function for the Add/Edit form component
void ButtonEditComponent::Draw() {
    
    ImGui::PushID(this);

    bool isEditing = !m_editingButtonId.empty();
    const char* headerText = isEditing ? m_translator.get("edit_button_header").c_str() : m_translator.get("add_new_button_header").c_str();

    // Wrap the entire component in a collapsing header, open by default
    if (ImGui::CollapsingHeader(headerText, ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Remove the separate ImGui::TextUnformatted for the header
        // ImGui::TextUnformatted(headerText);

        // --- Form Table ---
        if (ImGui::BeginTable("add_edit_button_form_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);

            // Row 1: Button ID
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_id_label").c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-FLT_MIN);
            ImGuiInputTextFlags idFlags = isEditing ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
            ImGui::InputText("##ButtonID_EditComp", m_newButtonId, sizeof(m_newButtonId), idFlags); // Unique ID for input
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered() && !isEditing) { ImGui::SetTooltip("%s", m_translator.get("button_id_tooltip").c_str()); }
            if (isEditing) { ImGui::SameLine(); ImGui::TextDisabled("(Cannot be changed)"); }

            // Row 2: Button Name
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_name_label").c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-FLT_MIN);
            // <<< ADDED: Debug log for button name buffer >>>
            // std::cout << "Drawing Button Name Input. Buffer content: [" << m_newButtonName << "]" << std::endl;
            ImGui::InputText("##ButtonName_EditComp", m_newButtonName, sizeof(m_newButtonName));
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_name_tooltip").c_str()); }

            // Row 3: Action Type
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("action_type_label").c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-FLT_MIN);
            std::vector<const char*> actionTypeDisplayItems;
            std::string previousActionType = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) ? m_supportedActionTypes[m_newButtonActionTypeIndex] : "";
            for(const auto& type : m_supportedActionTypes) {
                std::string translationKey = "action_type_" + type + "_display";
                actionTypeDisplayItems.push_back(m_translator.get(translationKey).c_str());
            }
            if (ImGui::Combo("##ActionTypeCombo_EditComp", &m_newButtonActionTypeIndex, actionTypeDisplayItems.data(), actionTypeDisplayItems.size())) {
                std::string newActionType = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) ? m_supportedActionTypes[m_newButtonActionTypeIndex] : "";
                if (previousActionType == "hotkey" && newActionType != "hotkey") m_isCapturingHotkey = false;
                if (newActionType.rfind("media_", 0) == 0) {
                    m_isCapturingHotkey = false;
                    m_newButtonActionParam[0] = '\\0';
                }
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                ImGui::SetTooltip("%s", m_translator.get("action_type_tooltip").c_str());
            }

            // Row 4: Action Param
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("action_param_label").c_str());
            ImGui::TableSetColumnIndex(1);
            { // Scope for Action Param logic
                std::string currentActionTypeStr = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size())
                                                   ? m_supportedActionTypes[m_newButtonActionTypeIndex] : "";
                bool isHotkeyAction = (currentActionTypeStr == "hotkey");
                bool isLaunchAppAction = (currentActionTypeStr == "launch_app");
                bool isMediaAction = (currentActionTypeStr.rfind("media_", 0) == 0);

                if (isMediaAction) {
                    ImGui::BeginDisabled(true);
                    char disabledText[] = "N/A"; // Shorter text
                    ImGui::PushItemWidth(-FLT_MIN);
                    ImGui::InputText("##ActionParamDisabled_EditComp", disabledText, sizeof(disabledText), ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopItemWidth();
                    ImGui::EndDisabled();
                     if (strlen(m_newButtonActionParam) > 0) { m_newButtonActionParam[0] = '\\0'; }
                } else if (isHotkeyAction) {
                     ImGui::Checkbox("##ManualHotkeyCheckbox_EditComp", &m_manualHotkeyEntry);
                     ImGui::SameLine(); ImGui::TextUnformatted(m_translator.get("hotkey_manual_input_checkbox").c_str());
                     if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_manual_input_tooltip").c_str()); }

                     ImGui::PushItemWidth(-FLT_MIN);
                     if (m_manualHotkeyEntry) {
                         m_isCapturingHotkey = false;
                         ImGui::InputText("##ActionParamInputManual_EditComp", m_newButtonActionParam, sizeof(m_newButtonActionParam));
                         if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }
                     } else {
                         if (m_isCapturingHotkey) {
                             char capturePlaceholder[256];
                             snprintf(capturePlaceholder, sizeof(capturePlaceholder), m_translator.get("hotkey_capture_prompt").c_str(), m_newButtonActionParam);
                             ImGui::InputText("##ActionParamInputCapturing_EditComp", capturePlaceholder, sizeof(capturePlaceholder), ImGuiInputTextFlags_ReadOnly);
                             if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_capturing").c_str()); }
                             if (InputUtils::TryCaptureHotkey(m_newButtonActionParam, sizeof(m_newButtonActionParam))) {
                                 m_isCapturingHotkey = false;
                             }
                             if (m_isCapturingHotkey && !ImGui::IsItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                                 ImGui::SetKeyboardFocusHere(-1);
                             }
                         } else {
                             ImGui::InputText("##ActionParamInput_EditComp", m_newButtonActionParam, sizeof(m_newButtonActionParam), ImGuiInputTextFlags_ReadOnly);
                             if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_start").c_str()); }
                             if (ImGui::IsItemClicked()) {
                                 m_isCapturingHotkey = true;
                                 m_newButtonActionParam[0] = '\\0';
                                 ImGui::SetKeyboardFocusHere(-1);
                             }
                         }
                     }
                     ImGui::PopItemWidth();
                } else { // launch_app, open_url, etc.
                     m_isCapturingHotkey = false;
                     float browseButtonWidth = 0.0f;
                     if (isLaunchAppAction) {
                         browseButtonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemSpacing.x * 2.0f; // Use "..." like icon browse
                     }
                     float inputWidth = ImGui::GetContentRegionAvail().x - browseButtonWidth;
                     ImGui::PushItemWidth(inputWidth > 0 ? inputWidth : -FLT_MIN);
                     ImGui::InputText("##ActionParamInputOther_EditComp", m_newButtonActionParam, sizeof(m_newButtonActionParam));
                     ImGui::PopItemWidth();
                     if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }

                     if (isLaunchAppAction) {
                         ImGui::SameLine();
                         if (ImGui::Button("...##AppBrowse_EditComp")) { // Consistent "..." button
                             const char* title = m_translator.get("select_app_dialog_title").c_str();
                             const char* key = "SelectAppDlgKey_BtnEdit"; // Key specific to this component
                             #ifdef _WIN32
                             const char* filters = ".exe{.exe}";
                             #else
                             const char* filters = ".*";
                             #endif
                             IGFD::FileDialogConfig config; config.path = ".";
                             ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
                         }
                     }
                }
            } // End Action Param scope

            // Row 5: Icon Path
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_icon_label").c_str());
            ImGui::TableSetColumnIndex(1);
            { // Scope for Icon Path logic
                float browseButtonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                float inputWidth = ImGui::GetContentRegionAvail().x - browseButtonWidth;
                ImGui::PushItemWidth(inputWidth > 0 ? inputWidth : -FLT_MIN);
                ImGui::InputText("##IconPath_EditComp", m_newButtonIconPath, sizeof(m_newButtonIconPath));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("...##IconBrowse_EditComp")) {
                     const char* title = m_translator.get("select_icon_dialog_title").c_str();
                     const char* key = "SelectIconDlgKey_BtnEdit"; // Key specific to this component
                     const char* filters = "Image files (*.png *.jpg *.jpeg *.bmp *.gif){.png,.jpg,.jpeg,.bmp,.gif},.*";
                     IGFD::FileDialogConfig config; config.path = ".";
                     ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
                }
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_icon_tooltip").c_str()); }
            } // End Icon Path scope

            ImGui::EndTable();
        } // End Form Table

        // --- Action Buttons --- (inside the collapsing header)
        const char* submitLabel = isEditing ? m_translator.get("save_changes_button_label").c_str() : m_translator.get("add_button_label").c_str();
        if (ImGui::Button(submitLabel)) {
            SubmitForm();
        }

        // Cancel button (only in edit mode)
        if (isEditing) {
            ImGui::SameLine();
            if (ImGui::Button(m_translator.get("cancel_button_label").c_str())) {
                 std::cout << "[ButtonEditComponent] Edit cancelled for button ID: " << m_editingButtonId << std::endl; // Optional log
                 m_editingButtonId = ""; // Just exit edit mode
                 m_isCapturingHotkey = false; // Reset capture state if active
                 m_manualHotkeyEntry = false; // Reset manual entry state
            }
        }

    } // End of CollapsingHeader

    // --- File Dialog Handling --- (outside the collapsing header)
    // Must be called every frame to handle potential dialog display
    HandleFileDialog();

    ImGui::PopID();
}