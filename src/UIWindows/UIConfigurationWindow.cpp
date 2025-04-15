#include "UIConfigurationWindow.hpp"
#include "../Managers/UIManager.hpp"
#include "../Utils/InputUtils.hpp" // Include necessary headers
#include <iostream>          // For std::cout, std::cerr
#include <cstring>           // For strncpy, strlen
#include <string>            // For std::string manipulation
#include <filesystem>        // For directory operations
#include <algorithm>         // For string manipulation (tolower)

namespace fs = std::filesystem;

UIConfigurationWindow::UIConfigurationWindow(UIManager& uiManager, ConfigManager& configManager, TranslationManager& translationManager)
    : m_uiManager(uiManager),
      m_configManager(configManager),
      m_translator(translationManager)
{
    loadCurrentSettings(); // Load initial layout settings
    scanPresetDirectory(); // Scan for presets on initialization
}

// Implementation of HandleFileDialog (moved from the end of drawConfigurationWindow)
void UIConfigurationWindow::HandleFileDialog() {
    std::string selectedFilePath;
    bool updateParam = false; // Combined flag for action param
    bool updateIcon = false;  // Combined flag for icon path

    // Define display settings for the dialog
    ImVec2 minSize = ImVec2(600, 400);
    ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);

    // Check which dialog was potentially opened and handle its display and result.
    // We use distinct keys to differentiate.

    // Dialog for selecting Application File (Action Param)
    if (ImGuiFileDialog::Instance()->Display("SelectAppDlgKey_Config", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            // No need to check user data if we use distinct keys
            updateParam = true; // Assume OK means update the action param
            std::cout << "Selected App Path: " << selectedFilePath << std::endl;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Dialog for selecting Icon File (Icon Path)
    if (ImGuiFileDialog::Instance()->Display("SelectIconDlgKey_Config", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            // No need to check user data if we use distinct keys
            updateIcon = true; // Assume OK means update the icon path
            std::cout << "Selected Icon Path: " << selectedFilePath << std::endl;
        }
        ImGuiFileDialog::Instance()->Close();
    }


    // Update the appropriate buffer outside the Display scope based on flags
    if (updateParam) {
         strncpy(m_newButtonActionParam, selectedFilePath.c_str(), sizeof(m_newButtonActionParam) - 1);
         m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = '\0'; // Ensure null termination
         std::cout << "Updated Action Param Buffer: " << m_newButtonActionParam << std::endl;
    }
    if (updateIcon) {
         strncpy(m_newButtonIconPath, selectedFilePath.c_str(), sizeof(m_newButtonIconPath) - 1);
         m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\0'; // Ensure null termination
         std::cout << "Updated Icon Path Buffer: " << m_newButtonIconPath << std::endl;
    }
}

void UIConfigurationWindow::loadCurrentSettings() {
    const auto& layout = m_configManager.getLayoutConfig();
    m_tempPageCount = layout.page_count;
    m_tempRowsPerPage = layout.rows_per_page;
    m_tempColsPerPage = layout.cols_per_page;
}

void UIConfigurationWindow::scanPresetDirectory() {
    m_presetFileNames.clear();
    m_selectedPresetIndex = -1; // Reset selection

    try {
        if (!fs::exists(m_presetsDir)) {
            std::cout << "[UIConfigWindow] Preset directory not found: " << m_presetsDir << ". No presets loaded." << std::endl;
            return; // Directory doesn't exist, nothing to scan
        }

        for (const auto& entry : fs::directory_iterator(m_presetsDir)) {
            if (entry.is_regular_file()) {
                fs::path filepath = entry.path();
                std::string extension = filepath.extension().string();
                // Simple case-insensitive check for .json
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".json") {
                    std::u8string u8Name = filepath.stem().u8string(); 
                    m_presetFileNames.push_back(std::string(u8Name.begin(), u8Name.end()));
                }
            }
        }
        std::cout << "[UIConfigWindow] Scanned presets directory. Found " << m_presetFileNames.size() << " preset files." << std::endl;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[UIConfigWindow] Filesystem error while scanning presets directory: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[UIConfigWindow] Error scanning presets directory: " << e.what() << std::endl;
    }
}

void UIConfigurationWindow::Draw() {
    ImGui::Begin(m_translator.get("config_window_title").c_str());

    const auto& buttons = m_configManager.getButtons();

    // --- Loaded Buttons Table ---
    if (buttons.empty()) {
        ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
    } else {
        ImGui::TextUnformatted(m_translator.get("loaded_buttons_header").c_str());
        if (ImGui::BeginTable("buttons_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            for (const auto& button : buttons) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", button.id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", button.name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(button.id.c_str());
                if (ImGui::SmallButton(m_translator.get("edit_button_label").c_str())) {
                    auto buttonToEditOpt = m_configManager.getButtonById(button.id);
                    if (buttonToEditOpt) {
                        const auto& btnCfg = *buttonToEditOpt;
                        m_editingButtonId = btnCfg.id;
                        strncpy(m_newButtonId, btnCfg.id.c_str(), sizeof(m_newButtonId) - 1); m_newButtonId[sizeof(m_newButtonId) - 1] = 0;
                        strncpy(m_newButtonName, btnCfg.name.c_str(), sizeof(m_newButtonName) - 1); m_newButtonName[sizeof(m_newButtonName) - 1] = 0;
                        m_newButtonActionTypeIndex = -1; // Reset before searching
                        for(size_t i = 0; i < m_supportedActionTypes.size(); ++i) {
                            if (m_supportedActionTypes[i] == btnCfg.action_type) {
                                m_newButtonActionTypeIndex = static_cast<int>(i);
                                break;
                            }
                        }
                        // Handle case where action type wasn't found (set to 0 or log error?)
                         if (m_newButtonActionTypeIndex == -1) {
                            std::cerr << "Warning: Action type '" << btnCfg.action_type << "' for button ID '" << btnCfg.id << "' not found in supported types. Defaulting to first type." << std::endl;
                             m_newButtonActionTypeIndex = 0; // Default to the first one
                         }

                        strncpy(m_newButtonActionParam, btnCfg.action_param.c_str(), sizeof(m_newButtonActionParam) - 1); m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = 0;
                        strncpy(m_newButtonIconPath, btnCfg.icon_path.c_str(), sizeof(m_newButtonIconPath) - 1); m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = 0;
                        m_isCapturingHotkey = false; // Ensure capture mode is off when starting edit
                        m_manualHotkeyEntry = false; // Reset manual entry flag
                        std::cout << "Editing button: " << m_editingButtonId << std::endl;
                    } else {
                         std::cerr << "Error: Could not find button data for ID: " << button.id << " to edit." << std::endl;
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(m_translator.get("delete_button_label").c_str())) {
                    m_buttonIdToDelete = button.id;
                    m_showDeleteConfirmation = true;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();

    // --- Add/Edit Section ---
    bool isEditing = !m_editingButtonId.empty();
    ImGui::TextUnformatted(isEditing ? m_translator.get("edit_button_header").c_str() : m_translator.get("add_new_button_header").c_str());

    if (ImGui::BeginTable("add_edit_form", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV )) {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);

        // Row 1: Button ID
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_id_label").c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN); // Stretch
        ImGuiInputTextFlags idFlags = isEditing ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
        ImGui::InputText("##ButtonID", m_newButtonId, sizeof(m_newButtonId), idFlags);
        ImGui::PopItemWidth();
        if (ImGui::IsItemHovered() && !isEditing) { ImGui::SetTooltip("%s", m_translator.get("button_id_tooltip").c_str()); }
         if (isEditing) { ImGui::SameLine(); ImGui::TextDisabled("(Cannot be changed)"); }


        // Row 2: Button Name
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_name_label").c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN); // Stretch
        ImGui::InputText("##ButtonName", m_newButtonName, sizeof(m_newButtonName));
        ImGui::PopItemWidth();
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_name_tooltip").c_str()); }

        // Row 3: Action Type
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("action_type_label").c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN); // Stretch
        std::vector<const char*> actionTypeDisplayItems;
        std::string previousActionType = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) ? m_supportedActionTypes[m_newButtonActionTypeIndex] : "";
        for(const auto& type : m_supportedActionTypes) {
            std::string translationKey = "action_type_" + type + "_display";
            actionTypeDisplayItems.push_back(m_translator.get(translationKey).c_str());
        }
        if (ImGui::Combo("##ActionTypeCombo", &m_newButtonActionTypeIndex, actionTypeDisplayItems.data(), actionTypeDisplayItems.size())) {
             // If the action type changed *away* from hotkey, disable capture mode.
             std::string newActionType = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) ? m_supportedActionTypes[m_newButtonActionTypeIndex] : "";
             if (previousActionType == "hotkey" && newActionType != "hotkey") {
                 m_isCapturingHotkey = false;
             }
              // If action type changed *away* from media, clear param? No, handled below.
              // If switching *to* media, clear param and disable capture mode.
             if (newActionType.rfind("media_", 0) == 0) {
                  m_isCapturingHotkey = false;
                  m_newButtonActionParam[0] = '\0'; // Clear param for media actions
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
        // Action Param Input Logic
        { // Scope for variables
            std::string currentActionTypeStr = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size())
                                               ? m_supportedActionTypes[m_newButtonActionTypeIndex]
                                               : "";
            bool isHotkeyAction = (currentActionTypeStr == "hotkey");
            bool isLaunchAppAction = (currentActionTypeStr == "launch_app");
            bool isMediaAction = (currentActionTypeStr.rfind("media_", 0) == 0);
            bool isUrlAction = (currentActionTypeStr == "open_url"); // Added for potential future use/tooltip

            // --- Input based on Type ---
            if (isMediaAction) {
                ImGui::BeginDisabled(true);
                char disabledText[] = "N/A (handled by action type)";
                ImGui::PushItemWidth(-FLT_MIN);
                ImGui::InputText("##ActionParamDisabled", disabledText, sizeof(disabledText), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
                ImGui::EndDisabled();
                 // Ensure param is cleared if we just switched TO media type
                 if (strlen(m_newButtonActionParam) > 0) { m_newButtonActionParam[0] = '\0'; }
            } else if (isHotkeyAction) {
                ImGui::Checkbox("##ManualHotkeyCheckbox", &m_manualHotkeyEntry);
                ImGui::SameLine(); ImGui::TextUnformatted(m_translator.get("hotkey_manual_input_checkbox").c_str());
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_manual_input_tooltip").c_str()); }

                ImGui::PushItemWidth(-FLT_MIN);
                if (m_manualHotkeyEntry) {
                    m_isCapturingHotkey = false; // Turn off capture if switching to manual
                    ImGui::InputText("##ActionParamInputManual", m_newButtonActionParam, sizeof(m_newButtonActionParam));
                    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }
                } else {
                    if (m_isCapturingHotkey) {
                        char capturePlaceholder[256];
                        snprintf(capturePlaceholder, sizeof(capturePlaceholder), m_translator.get("hotkey_capture_prompt").c_str(), m_newButtonActionParam);
                        ImGui::InputText("##ActionParamInputCapturing", capturePlaceholder, sizeof(capturePlaceholder), ImGuiInputTextFlags_ReadOnly);
                        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_capturing").c_str()); }

                        // Attempt to capture the hotkey
                        if (InputUtils::TryCaptureHotkey(m_newButtonActionParam, sizeof(m_newButtonActionParam))) {
                            m_isCapturingHotkey = false; // Capture finished (success or escape)
                        }
                        // Keep focus for capture
                        if (m_isCapturingHotkey && !ImGui::IsItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                             ImGui::SetKeyboardFocusHere(-1);
                        }
                    } else {
                        ImGui::InputText("##ActionParamInput", m_newButtonActionParam, sizeof(m_newButtonActionParam), ImGuiInputTextFlags_ReadOnly); // Readonly until clicked
                        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_start").c_str()); }
                        if (ImGui::IsItemClicked()) {
                             m_isCapturingHotkey = true;
                             m_newButtonActionParam[0] = '\0'; // Clear buffer on starting capture
                             ImGui::SetKeyboardFocusHere(-1); // Focus the (invisible) input
                         }
                    }
                }
                ImGui::PopItemWidth();
            } else { // Handles launch_app, open_url, etc.
                 m_isCapturingHotkey = false; // Ensure capture is off
                 float browseButtonWidth = 0.0f;
                 if (isLaunchAppAction) {
                     browseButtonWidth = ImGui::CalcTextSize(m_translator.get("browse_button_label").c_str()).x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                 }
                 float inputWidth = ImGui::GetContentRegionAvail().x - browseButtonWidth;
                 ImGui::PushItemWidth(inputWidth > 0 ? inputWidth : -FLT_MIN);
                 ImGui::InputText("##ActionParamInput", m_newButtonActionParam, sizeof(m_newButtonActionParam));
                 ImGui::PopItemWidth();
                 if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }

                 if (isLaunchAppAction) {
                     ImGui::SameLine();
                     if (ImGui::Button(m_translator.get("browse_button_label").c_str())) {
                         const char* title = m_translator.get("select_app_dialog_title").c_str();
                         // Use a key specific to this window instance/purpose
                         const char* key = "SelectAppDlgKey_Config";
                         #ifdef _WIN32
                         const char* filters = ".exe{.exe}"; // Windows specific filter
                         #else
                         const char* filters = ".*"; // More generic filter
                         #endif
                         IGFD::FileDialogConfig config; config.path = ".";
                         // config.userDatas = (void*)"ConfigWindowActionParam"; // Not strictly needed if using unique keys
                         ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
                          std::cout << "Opening File Dialog: " << key << std::endl;
                     }
                 }
            }
        } // End Action Param scope


        // Row 5: Icon Path
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(m_translator.get("button_icon_label").c_str());
        ImGui::TableSetColumnIndex(1);
        // Icon Path Input Logic
        {
            float browseButtonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
            float inputWidth = ImGui::GetContentRegionAvail().x - browseButtonWidth;
            ImGui::PushItemWidth(inputWidth > 0 ? inputWidth : -FLT_MIN);
            ImGui::InputText("##IconPath", m_newButtonIconPath, sizeof(m_newButtonIconPath));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("...##IconBrowse")) {
                 const char* title = m_translator.get("select_icon_dialog_title").c_str(); // Use a specific title key
                 const char* key = "SelectIconDlgKey_Config"; // Use a key specific to this window instance/purpose
                 // More specific image filters
                 const char* filters = "Image files (*.png *.jpg *.jpeg *.bmp *.gif){.png,.jpg,.jpeg,.bmp,.gif},.*";
                 IGFD::FileDialogConfig config; config.path = ".";
                 // config.userDatas = (void*)"ConfigWindowIconPath"; // Not strictly needed if using unique keys
                 ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
                 std::cout << "Opening File Dialog: " << key << std::endl;

            }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_icon_tooltip").c_str()); }
        } // End Icon Path scope


        ImGui::EndTable();
    } // End Add/Edit Form Table


    // --- Action Buttons (Add/Save/Cancel) ---
    const char* submitLabel = isEditing ? m_translator.get("save_changes_button_label").c_str() : m_translator.get("add_button_label").c_str();
    bool submitted = ImGui::Button(submitLabel);

    // Cancel button (only in edit mode)
    if (isEditing) {
        ImGui::SameLine();
        if (ImGui::Button(m_translator.get("cancel_button_label").c_str())) {
             // Clear fields and exit edit mode
             m_newButtonId[0] = '\0'; m_newButtonName[0] = '\0'; m_newButtonActionTypeIndex = -1;
             m_newButtonActionParam[0] = '\0'; m_newButtonIconPath[0] = '\0';
             std::string cancelledId = m_editingButtonId; // Store before clearing
             m_editingButtonId = ""; // Exit edit mode
             m_isCapturingHotkey = false; // Ensure capture is off
             std::cout << "Edit cancelled for button ID: " << cancelledId << std::endl;
             submitted = false; // Don't process submit if cancelled
        }
    }

    if (submitted) {
        ButtonConfig buttonData;
        buttonData.name = m_newButtonName;
        if (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) {
             buttonData.action_type = m_supportedActionTypes[m_newButtonActionTypeIndex];
        } else {
            buttonData.action_type = ""; // Should not happen if index is validated/defaulted
            std::cerr << "Error: Invalid action type index during submit." << std::endl;
        }
        buttonData.action_param = m_newButtonActionParam;
         // For media actions, ensure param is empty in the config
         if (buttonData.action_type.rfind("media_", 0) == 0) {
             buttonData.action_param = "";
         }
        buttonData.icon_path = m_newButtonIconPath;

        bool configChanged = false;
        bool saveSuccess = false;

        if (isEditing) {
            // --- Update Logic ---
            buttonData.id = m_editingButtonId; // ID is read-only, use stored one
            if (buttonData.name.empty()) {
                std::cerr << "Error: Updated button name cannot be empty." << std::endl;
                // TODO: Show user feedback in UI?
            } else if (m_configManager.updateButton(m_editingButtonId, buttonData)) {
                std::cout << "Button updated successfully: " << m_editingButtonId << std::endl;
                configChanged = true;
            } else {
                 std::cerr << "Error: Failed to update button " << m_editingButtonId << " (check console/ConfigManager logs)." << std::endl;
                 // TODO: Show user feedback in UI?
            }
        } else {
            // --- Add Logic ---
            buttonData.id = m_newButtonId;
            if (buttonData.id.empty() || buttonData.name.empty()) {
                std::cerr << "Error: Cannot add button with empty ID or Name." << std::endl;
                // TODO: Show user feedback in UI?
            } else if (m_configManager.addButton(buttonData)) {
                std::cout << m_translator.get("button_added_success_log") << buttonData.id << std::endl;
                 configChanged = true;
            } else {
                std::cerr << m_translator.get("add_button_fail_log") << " ID: " << buttonData.id << std::endl;
                // TODO: Show user feedback in UI? Could be duplicate ID.
            }
        }

        if (configChanged) {
             if (m_configManager.saveConfig()) {
                 std::cout << "Configuration saved successfully." << std::endl;
                 saveSuccess = true;
             } else {
                 std::cerr << "Error: Failed to save configuration after changes." << std::endl;
                 // TODO: Show user feedback in UI?
             }
        }

        // If add/update and save were successful, clear the form and exit edit mode
        if (configChanged && saveSuccess) {
             m_newButtonId[0] = '\0'; m_newButtonName[0] = '\0'; m_newButtonActionTypeIndex = -1;
             m_newButtonActionParam[0] = '\0'; m_newButtonIconPath[0] = '\0';
             m_editingButtonId = ""; // Exit edit mode if we were editing
             m_isCapturingHotkey = false; // Ensure capture is off
        }
    } // End if submitted


    // --- Delete Confirmation Modal ---
    if (m_showDeleteConfirmation) {
        ImGui::OpenPopup(m_translator.get("delete_confirm_title").c_str());
        m_showDeleteConfirmation = false; // Reset flag after opening
    }

    if (ImGui::BeginPopupModal(m_translator.get("delete_confirm_title").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(m_translator.get("delete_confirm_text").c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s", m_buttonIdToDelete.c_str());
        ImGui::Separator();

        bool deleted = false;
        if (ImGui::Button(m_translator.get("delete_confirm_yes").c_str(), ImVec2(120, 0))) {
            if (m_configManager.removeButton(m_buttonIdToDelete)) {
                 std::cout << m_translator.get("button_removed_log") << m_buttonIdToDelete << std::endl;
                 if (m_configManager.saveConfig()) {
                     std::cout << m_translator.get("config_saved_delete_log") << std::endl;
                     deleted = true; // Indicate success
                 } else {
                     std::cerr << m_translator.get("config_save_fail_delete_log") << std::endl;
                     // Consider how to handle this - maybe revert the delete? For now, log error.
                 }
            } else {
                 std::cerr << m_translator.get("remove_button_fail_log") << m_buttonIdToDelete
                           << m_translator.get("remove_button_fail_log_suffix") << std::endl;
            }
            m_buttonIdToDelete = ""; // Clear ID regardless of success for this popup
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(m_translator.get("delete_confirm_cancel").c_str(), ImVec2(120, 0))) {
            std::cout << m_translator.get("delete_cancel_log") << m_buttonIdToDelete << std::endl;
            m_buttonIdToDelete = "";
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();

         // If deletion occurred AND we were editing the *same* button, cancel the edit.
         if (deleted && !m_editingButtonId.empty() && m_editingButtonId == m_buttonIdToDelete /* This check is redundant as m_buttonIdToDelete is cleared, but conceptually correct */ ) {
             m_newButtonId[0] = '\0'; m_newButtonName[0] = '\0'; m_newButtonActionTypeIndex = -1;
             m_newButtonActionParam[0] = '\0'; m_newButtonIconPath[0] = '\0';
             m_editingButtonId = "";
             m_isCapturingHotkey = false;
             std::cout << "Cancelled edit mode because the button being edited was deleted." << std::endl;
         }

    } // End Delete Modal


    // --- File Dialog Handling ---
    // Call the helper function to display and handle results for *all* file dialogs this window might use.
    HandleFileDialog();

    ImGui::Separator(); // Add a separator before the preset section
    ImGui::TextUnformatted(m_translator.get("preset_management_header").c_str());

    // --- Preset Loading ---
    ImGui::TextUnformatted(m_translator.get("load_preset_label").c_str());
    ImGui::SameLine();

    // Need to convert vector<string> to const char* const* for ImGui::Combo
    std::vector<const char*> presetItems;
    for (const auto& name : m_presetFileNames) {
        presetItems.push_back(name.c_str());
    }

    // Combo box to select preset
    ImGui::PushItemWidth(200); // Adjust width as needed
    if (ImGui::Combo("##PresetSelect", &m_selectedPresetIndex, presetItems.data(), presetItems.size())) {
        // Selection changed
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Button to load the selected preset
    if (ImGui::Button(m_translator.get("load_selected_button").c_str())) {
        if (m_selectedPresetIndex >= 0 && m_selectedPresetIndex < m_presetFileNames.size()) {
            std::string selectedPresetName = m_presetFileNames[m_selectedPresetIndex];
            std::string fullPath = m_presetsDir + "/" + selectedPresetName + ".json";
            std::cout << "[UIConfigWindow] Attempting to load preset: " << fullPath << std::endl;
            if (m_configManager.loadConfigFromFile(fullPath)) {
                std::cout << "[UIConfigWindow] Preset loaded successfully. Updating UI and notifying." << std::endl;
                loadCurrentSettings(); // Update temp variables in this window
                // Assuming m_uiManager exists and is valid
                m_uiManager.notifyLayoutChanged(); 
            } else {
                std::cerr << "[UIConfigWindow] Failed to load preset: " << fullPath << std::endl;
                // TODO: Show error message in UI
            }
        } else {
            std::cout << "[UIConfigWindow] No preset selected or index out of bounds." << std::endl;
             // TODO: Show message if nothing is selected
        }
    }
    ImGui::SameLine();

    // Button to refresh the preset list
    if (ImGui::Button(m_translator.get("refresh_list_button").c_str())) {
        scanPresetDirectory();
    }

    ImGui::Spacing(); // Add some space

    // --- Preset Saving ---
    ImGui::TextUnformatted(m_translator.get("save_preset_label").c_str());
    ImGui::SameLine();

    // Input field for the new preset name
    ImGui::PushItemWidth(200); // Adjust width
    ImGui::InputText("##NewPresetName", m_newPresetNameBuffer, sizeof(m_newPresetNameBuffer));
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Button to save the current config as a new preset
    if (ImGui::Button(m_translator.get("save_as_button").c_str())) {
        std::string newName = m_newPresetNameBuffer;
        // Basic validation: Trim whitespace? Check if empty?
        if (!newName.empty()) { // Add more robust validation if needed
             std::cout << "[UIConfigWindow] Attempting to save current config as preset: " << newName << std::endl;
            if (m_configManager.saveConfigToPreset(newName)) {
                std::cout << "[UIConfigWindow] Preset saved successfully. Refreshing list." << std::endl;
                scanPresetDirectory(); // Refresh the list to include the new preset
                m_newPresetNameBuffer[0] = '\0'; // Clear the input buffer
                // TODO: Provide success feedback in UI
            } else {
                 std::cerr << "[UIConfigWindow] Failed to save preset: " << newName << std::endl;
                 // TODO: Show error message in UI
            }
        } else {
             std::cout << "[UIConfigWindow] Preset name cannot be empty." << std::endl;
              // TODO: Show message if name is empty
        }
    }

    ImGui::Separator(); // Separator after the preset section

    // --- Button List Section ---
    ImGui::Separator(); // Separator before button list

    ImGui::End(); // End Configuration Window
}