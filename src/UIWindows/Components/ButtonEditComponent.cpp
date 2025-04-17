// src/UIWindows/Components/ButtonEditComponent.cpp
#include "ButtonEditComponent.hpp"
#include <imgui.h>
#include <iostream> // For logging
#include <cstring>  // For strncpy, memset
#include <algorithm> // For std::find, std::replace, std::string conversion
#include <string>    // For std::string
#include <nfd.h>     // <<< ADDED: Include NFD header
#include "../../Utils/InputUtils.hpp" // <<< ADDED: Include InputUtils for hotkey capture
#include "../../Utils/StringUtils.hpp"
#include <filesystem> // <<< ADDED: For path operations

// #include <ImGuiFileDialog.h> // <<< REMOVED (or ensure it's removed from ButtonEditComponent.hpp too)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // For GetModuleFileNameW, MAX_PATH
#endif

namespace fs = std::filesystem;

// <<< ADDED: Helper function to get executable directory (Windows) >>>
#ifdef _WIN32
std::wstring GetExecutableDirectoryW() {
    wchar_t path[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
        std::cerr << "[GetExecutableDirectoryW] GetModuleFileNameW failed. Error code: " << GetLastError() << std::endl;
        return L"";
    }
    try {
        fs::path exePath(path);
        return exePath.parent_path().wstring();
    } catch (const std::exception& e) {
        std::wcerr << L"[GetExecutableDirectoryW] Filesystem error getting parent path from: " << path << L". Error: " << e.what() << std::endl;
        return L"";
    }
}
#else
// TODO: Implement for other platforms if needed
std::string GetExecutableDirectory() {
    // Placeholder for non-Windows
    // Could use methods from <filesystem> if available and suitable,
    // or platform-specific APIs (e.g., readlink("/proc/self/exe") on Linux)
    std::cerr << "[GetExecutableDirectory] Not implemented for this platform." << std::endl;
    return "."; // Return current directory as a fallback
}
#endif

// Constructor
ButtonEditComponent::ButtonEditComponent(ConfigManager& configManager, TranslationManager& translator)
    : m_configManager(configManager),
      m_translator(translator)
{}

// Public method called by parent window to initiate editing
void ButtonEditComponent::StartEdit(const ButtonConfig& btnCfg) {
    std::cout << "[ButtonEditComponent] Starting edit for ID: " << btnCfg.id << std::endl;
    ClearForm(); // Clear first to reset everything
    m_editingButtonId = btnCfg.id; // Mark as editing *existing*
    m_addingNew = false;          // Explicitly not adding new

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
    if (m_newButtonActionTypeIndex == -1) {
        std::cerr << "[ButtonEditComponent] Warning: Action type '" << btnCfg.action_type << "' for button ID '" << btnCfg.id << "' not found. Defaulting." << std::endl;
        m_newButtonActionTypeIndex = 0; // Default to the first type
    }

    m_isCapturingHotkey = false; // Ensure capture mode is off when starting edit
    m_manualHotkeyEntry = false; // Reset manual entry flag
    std::cout << "[ButtonEditComponent] Started editing button: " << m_editingButtonId << std::endl;
}

// <<< ADDED: Implementation for starting add with prefilled data >>>
void ButtonEditComponent::StartAddNewPrefilled(const PrefilledButtonData& data) {
    std::cout << "[ButtonEditComponent] Starting add new prefilled. Suggested ID: " << data.suggested_id << std::endl;
    ClearForm(); // Clear first
    m_editingButtonId = ""; // Ensure not in edit mode
    m_addingNew = true;     // Set adding flag

    // Copy prefilled data, ensuring null termination
    strncpy(m_newButtonId, data.suggested_id.c_str(), sizeof(m_newButtonId) - 1);
    m_newButtonId[sizeof(m_newButtonId) - 1] = '\0';

    strncpy(m_newButtonName, data.suggested_name.c_str(), sizeof(m_newButtonName) - 1);
    m_newButtonName[sizeof(m_newButtonName) - 1] = '\0';

    strncpy(m_newButtonActionParam, data.action_param.c_str(), sizeof(m_newButtonActionParam) - 1);
    m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = '\0';

    strncpy(m_newButtonIconPath, data.suggested_icon_path.c_str(), sizeof(m_newButtonIconPath) - 1);
    m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\0';

    // Find action type index
    m_newButtonActionTypeIndex = -1;
    auto it = std::find(m_supportedActionTypes.begin(), m_supportedActionTypes.end(), data.action_type);
    if (it != m_supportedActionTypes.end()) {
        m_newButtonActionTypeIndex = static_cast<int>(std::distance(m_supportedActionTypes.begin(), it));
    } else {
        std::cerr << "[ButtonEditComponent] Warning: Prefilled action type '" << data.action_type << "' not supported. Defaulting." << std::endl;
        m_newButtonActionTypeIndex = 0; // Default to first type
    }
     // m_isCapturingHotkey and m_manualHotkeyEntry are reset by ClearForm()
}

// Helper to clear the form fields and exit edit mode
void ButtonEditComponent::ClearForm() {
    memset(m_newButtonId, 0, sizeof(m_newButtonId));
    memset(m_newButtonName, 0, sizeof(m_newButtonName));
    m_newButtonActionTypeIndex = -1; // Reset combo box selection
    memset(m_newButtonActionParam, 0, sizeof(m_newButtonActionParam));
    memset(m_newButtonIconPath, 0, sizeof(m_newButtonIconPath));
    std::string cancelledId = m_editingButtonId;
    m_editingButtonId = ""; // Exit edit mode
    m_isCapturingHotkey = false;
    m_manualHotkeyEntry = false;
    m_addingNew = false; // Reset adding flag too
    if (!cancelledId.empty()) {
        std::cout << "[ButtonEditComponent] Edit cancelled/cleared for button ID: " << cancelledId << std::endl;
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
    if (buttonData.action_type.rfind("media_", 0) == 0 || buttonData.action_type.rfind("play_", 0) == 0) {
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

    // If add/update was successful, clear the form immediately
    if (configChanged) {
        ClearForm();

        // Attempt to save config
        if (m_configManager.saveConfig()) {
            std::cout << "[ButtonEditComponent] Configuration saved successfully." << std::endl;
            saveSuccess = true;
        } else {
            std::cerr << "[ButtonEditComponent] Error: Failed to save configuration after changes." << std::endl;
        }
    }

    // Form is now cleared if configChanged was true, regardless of saveSuccess.
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
                if (newActionType.rfind("media_", 0) == 0 || newActionType.rfind("play_", 0) == 0) {
                    m_isCapturingHotkey = false;
                    m_newButtonActionParam[0] = '\0';
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
                bool isPlaySoundAction = (currentActionTypeStr.rfind("play_", 0) == 0);

                if (isMediaAction || isPlaySoundAction) {
                    ImGui::BeginDisabled(true);
                    char disabledText[] = "N/A"; // Shorter text
                    ImGui::PushItemWidth(-FLT_MIN);
                    ImGui::InputText("##ActionParamDisabled_EditComp", disabledText, sizeof(disabledText), ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopItemWidth();
                    ImGui::EndDisabled();
                     if (strlen(m_newButtonActionParam) > 0) { m_newButtonActionParam[0] = '\0'; }
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
                                 m_newButtonActionParam[0] = '\0';
                                 ImGui::SetKeyboardFocusHere(-1);
                             }
                         }
                     }
                     ImGui::PopItemWidth();
                } else { // launch_app, open_url, etc.
                     m_isCapturingHotkey = false;
                     float browseButtonWidth = 0.0f;
                     if (isLaunchAppAction) {
                         browseButtonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                     }
                     float inputWidth = ImGui::GetContentRegionAvail().x - browseButtonWidth;
                     ImGui::PushItemWidth(inputWidth > 0 ? inputWidth : -FLT_MIN);
                     
                     ImGui::InputText("##ActionParamInputOther_EditComp", m_newButtonActionParam, sizeof(m_newButtonActionParam));

                     ImGui::PopItemWidth();
                     if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }

                     if (isLaunchAppAction) {
                         ImGui::SameLine();
                         if (ImGui::Button("...##AppBrowse_EditComp")) { // Consistent "..." button
                             nfdchar_t *outPath = NULL;
                             #ifdef _WIN32
                             nfdfilteritem_t filt[3] = { { "Executable files", "exe" }, { "Batch files", "bat" }, { "Command files", "cmd" } };
                             #else
                             nfdfilteritem_t filt[1] = { { "All files", "*" } }; // Simpler filter for non-windows
                             #endif
                             nfdresult_t result = NFD_OpenDialog(&outPath, filt, sizeof(filt)/sizeof(nfdfilteritem_t), NULL); // NULL for default path

                             if (result == NFD_OKAY) {
                                 std::cout << "[ButtonEditComponent] App selected: " << outPath << std::endl;
                                 strncpy(m_newButtonActionParam, outPath, sizeof(m_newButtonActionParam) - 1);
                                 m_newButtonActionParam[sizeof(m_newButtonActionParam) - 1] = '\0'; // Ensure null termination
                                 NFD_FreePath(outPath); // IMPORTANT: Free the path returned by NFD
                             } else if (result == NFD_CANCEL) {
                                 std::cout << "[ButtonEditComponent] App selection cancelled." << std::endl;
                             } else { // NFD_ERROR or unexpected
                                 std::cerr << "[ButtonEditComponent] Error selecting app: " << NFD_GetError() << std::endl;
                             }
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
                
                // <<< ADDED: Normalize icon path slashes for display >>>
                char displayBufferIcon[sizeof(m_newButtonIconPath)];
                strncpy(displayBufferIcon, m_newButtonIconPath, sizeof(displayBufferIcon) - 1);
                displayBufferIcon[sizeof(displayBufferIcon) - 1] = '\0'; // Ensure null termination
                
                // Replace forward slashes with backslashes for display
                for (char* p = displayBufferIcon; *p != '\0'; ++p) {
                    if (*p == '/') {
                        *p = '\\'; // Display 
                    }   
                }
                
                // Use the display buffer in InputText
                bool iconEdited = ImGui::InputText("##IconPath_EditComp", displayBufferIcon, sizeof(displayBufferIcon));
                
                // If edited, normalize back to forward slashes and save to original buffer
                if (iconEdited) {
                    std::string editedIconStr = displayBufferIcon;
                    std::replace(editedIconStr.begin(), editedIconStr.end(), '\\', '/'); // Normalize back to forward slashes
                    strncpy(m_newButtonIconPath, editedIconStr.c_str(), sizeof(m_newButtonIconPath) - 1);
                    m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\0'; // Ensure null termination
                }
                // <<< END ADDED >>>

                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("...##IconBrowse_EditComp")) {
                    nfdchar_t *outPathIcon = NULL;
                    nfdfilteritem_t filt_icon[2] = {
                        { "Image Files", "png,jpg,jpeg,bmp,gif" },
                        { "All Files", "*" }
                    };
                    nfdresult_t resultIcon = NFD_OpenDialog(&outPathIcon, filt_icon, sizeof(filt_icon)/sizeof(nfdfilteritem_t), NULL);

                    if (resultIcon == NFD_OKAY) {
                         // outPathIcon is char* (UTF-8)
                         std::cout << "[ButtonEditComponent] Icon selected (absolute): " << outPathIcon << std::endl;
                         bool path_set_successfully = false;
                         try {
                             #ifdef _WIN32
                             // *** Convert UTF-8 from NFD to wstring ***
                             std::wstring iconPathW = StringUtils::Utf8ToWide(outPathIcon);
                             if (iconPathW.empty() && outPathIcon && outPathIcon[0] != '\0') {
                                std::cerr << "[ButtonEditComponent] Failed to convert selected path to wstring." << std::endl;
                             } else {
                                 // *** Use the wstring (iconPathW) for filesystem ops ***
                                 fs::path iconPathAbs = iconPathW;
                                 std::wstring exeDirW = GetExecutableDirectoryW();
                                 std::wstring targetSubDirW = L"assets/icons";

                                 if (!exeDirW.empty()) {
                                     fs::path exePath = exeDirW;
                                     fs::path targetDirAbs = exePath / targetSubDirW;
                                     fs::path iconFilename = iconPathAbs.filename();
                                     fs::path destPathAbs = targetDirAbs / iconFilename;

                                     // Ensure target directory exists
                                     fs::create_directories(targetDirAbs);

                                     bool already_in_target = false;
                                     try {
                                        already_in_target = fs::exists(destPathAbs) && fs::equivalent(iconPathAbs, destPathAbs);
                                     } catch (const fs::filesystem_error& eq_err) {
                                        std::cerr << "[ButtonEditComponent] Filesystem error during equivalent check: " << eq_err.what() << std::endl;
                                        // Assume not equivalent if check fails
                                     }

                                     bool copy_needed = !already_in_target;
                                     bool copy_successful = true; // Assume success if not needed

                                     if (copy_needed) {
                                         std::wcout << L"[ButtonEditComponent] Icon not in target directory, attempting copy to: " << destPathAbs.wstring() << std::endl;
                                         try {
                                            fs::copy_file(iconPathAbs, destPathAbs, fs::copy_options::overwrite_existing);
                                            std::wcout << L"[ButtonEditComponent] Icon copy successful." << std::endl;
                                         } catch (const fs::filesystem_error& copy_err) {
                                            std::cerr << "[ButtonEditComponent] Filesystem error copying icon: " << copy_err.what() << std::endl;
                                            copy_successful = false;
                                         }
                                     } else {
                                         std::wcout << L"[ButtonEditComponent] Icon already in target directory: " << destPathAbs.wstring() << std::endl;
                                     }

                                     // Store the relative path ONLY if file is in target (original or copied)
                                     if (already_in_target || copy_successful) {
                                         std::wstring iconFilenameW = iconFilename.wstring();
                                         std::wstring relativePathW = targetSubDirW + L"/" + iconFilenameW;
                                         std::string relativePathStrUtf8 = StringUtils::WideToUtf8(relativePathW);

                                         std::cout << "[ButtonEditComponent] Storing relative path: " << relativePathStrUtf8 << std::endl;
                                         strncpy(m_newButtonIconPath, relativePathStrUtf8.c_str(), sizeof(m_newButtonIconPath) - 1);
                                         m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\0'; // Ensure null termination
                                         path_set_successfully = true;
                                     } else {
                                          std::cerr << "[ButtonEditComponent] Failed to ensure icon file exists in target directory. Path not updated." << std::endl;
                                     }

                                 } else {
                                     std::cerr << "[ButtonEditComponent] Could not get executable directory. Cannot process icon path." << std::endl;
                                 }
                             }
                             #else
                             // On non-Windows, assume NFD returns UTF-8 compatible with filesystem
                             fs::path iconPathAbs = outPathIcon;
                             std::string exeDir = GetExecutableDirectory();
                             std::wstring targetSubDirW = L"assets/icons";
                             fs::path exePath = exeDir;
                             fs::path targetDirAbs = exePath / targetSubDirW;
                             fs::path iconFilename = iconPathAbs.filename();
                              
                             // Ensure target directory exists
                             fs::create_directories(targetDirAbs);

                             bool already_in_target = false;
                             try {
                                already_in_target = fs::exists(destPathAbs) && fs::equivalent(iconPathAbs, destPathAbs);
                             } catch (const fs::filesystem_error& eq_err) {
                                std::cerr << "[ButtonEditComponent] Filesystem error during equivalent check: " << eq_err.what() << std::endl;
                             }

                             bool copy_needed = !already_in_target;
                             bool copy_successful = true;

                             if (copy_needed) {
                                 std::cout << "[ButtonEditComponent] Icon not in target directory, attempting copy to: " << destPathAbs.string() << std::endl;
                                 try {
                                    fs::copy_file(iconPathAbs, destPathAbs, fs::copy_options::overwrite_existing);
                                    std::cout << "[ButtonEditComponent] Icon copy successful." << std::endl;
                                 } catch (const fs::filesystem_error& copy_err) {
                                    std::cerr << "[ButtonEditComponent] Filesystem error copying icon: " << copy_err.what() << std::endl;
                                    copy_successful = false;
                                 }
                             } else {
                                 std::cout << "[ButtonEditComponent] Icon already in target directory: " << destPathAbs.string() << std::endl;
                             }

                             if (already_in_target || copy_successful) {
                                 std::wstring iconFilenameW = iconFilename.wstring();
                                 std::wstring relativePathW = targetSubDirW + L"/" + iconFilenameW;
                                 std::string relativePathStrUtf8 = StringUtils::WideToUtf8(relativePathW);

                                 std::cout << "[ButtonEditComponent] Storing relative path: " << relativePathStrUtf8 << std::endl;
                                 strncpy(m_newButtonIconPath, relativePathStrUtf8.c_str(), sizeof(m_newButtonIconPath) - 1);
                                 m_newButtonIconPath[sizeof(m_newButtonIconPath) - 1] = '\0';
                                 path_set_successfully = true;
                              } else {
                                   std::cerr << "[ButtonEditComponent] Failed to ensure icon file exists in target directory. Path not updated." << std::endl;
                              }
                             #endif
                         } catch (const fs::filesystem_error& e) {
                            // Handle filesystem errors not caught by specific checks
                            std::cerr << "[ButtonEditComponent] General filesystem error processing icon: " << e.what() << std::endl;
                         } catch (const std::exception& e) {
                             std::cerr << "[ButtonEditComponent] Error processing selected icon path: " << e.what() << std::endl;
                         }

                         // Free the original char* path
                         if (outPathIcon) {
                             NFD_FreePath(outPathIcon);
                         }
                         // Clear buffer if path setting failed, maybe?
                         // if (!path_set_successfully) { m_newButtonIconPath[0] = '\0'; }

                    } else if (resultIcon == NFD_CANCEL) {
                        std::cout << "[ButtonEditComponent] Icon selection cancelled." << std::endl;
                    } else { // NFD_ERROR or unexpected
                         std::cerr << "[ButtonEditComponent] Error selecting icon: " << NFD_GetError() << std::endl;
                    }
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

    ImGui::PopID();
    
}