// src/UIWindows/Components/ButtonListComponent.cpp
#include "ButtonListComponent.hpp"
#include <imgui.h>
#include <iostream> // For std::cout, std::cerr
#include <filesystem> // For path operations
#include <algorithm>  // For std::transform
#include <cctype>     // For std::tolower
#include <string>
#include <vector>
#include <iomanip> // For std::hex
#include <sstream> // For std::wstringstream
#include "../../Utils/IconUtils.hpp" // <<< ADDED: Include IconUtils

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // For WideCharToMultiByte
#include <io.h>      // For _waccess
#endif

namespace fs = std::filesystem;

// <<< Make sure the extern declaration is accessible if needed, or rely on header >>>
// extern std::vector<std::string> g_DroppedFiles;

// <<< ADDED: Helper function for Windows: Convert wstring to UTF-8 string >>>
#ifdef _WIN32
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) { // Handle error
         std::cerr << "[WideToUtf8] WideCharToMultiByte failed to calculate size. Error code: " << GetLastError() << std::endl;
         return ""; 
    }
    std::string strTo(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
     if (bytes_converted <= 0) { // Handle error
         std::cerr << "[WideToUtf8] WideCharToMultiByte failed to convert. Error code: " << GetLastError() << std::endl;
         return ""; 
    }
    return strTo;
}

std::wstring WStringToHex(const std::wstring& input) {
    std::wstringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (wchar_t wc : input) {
        ss << L" 0x" << std::setw(4) << static_cast<unsigned int>(wc);
    }
    return ss.str();
}
#endif

ButtonListComponent::ButtonListComponent(ConfigManager& configManager, TranslationManager& translator, EditRequestCallback onEditRequested, AddRequestCallback onAddRequested)
    : m_configManager(configManager),
      m_translator(translator),
      m_onEditRequested(std::move(onEditRequested)),
      m_onAddRequested(std::move(onAddRequested)) // Store the add callback
{}

void ButtonListComponent::Draw() {
    // <<< REMOVED: Platform-dependent check and processing is now moved >>>
    /*
    #ifdef _WIN32
    if (!g_DroppedFilesW.empty()) {
        ...
    }
    #else
    if (!g_DroppedFiles.empty()) {
        ...
    }
    #endif
    */
    // <<< END OF REMOVED PROCESSING >>>

    const auto& buttons = m_configManager.getButtons();

    std::string headerLabel = m_translator.get("loaded_buttons_header");
    headerLabel += " (" + std::to_string(buttons.size()) + ")";

    if (ImGui::CollapsingHeader(headerLabel.c_str())) {
        
        // --- Loaded Buttons Table (now directly inside CollapsingHeader) ---
        if (buttons.empty()) {
            ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
            ImGui::TextDisabled(m_translator.get("drag_drop_hint_text").c_str()); // Keep hint
        } else {
            if (ImGui::BeginTable("buttons_list_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
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
                    ImGui::PushID(button.id.c_str()); // Push ID for button uniqueness

                    // Edit Button
                    if (ImGui::SmallButton(m_translator.get("edit_button_label").c_str())) {
                        if (m_onEditRequested) { // Check if callback is valid
                            m_onEditRequested(button.id); // Trigger the callback
                        }
                    }
                    ImGui::SameLine();

                    // Delete Button
                    if (ImGui::SmallButton(m_translator.get("delete_button_label").c_str())) {
                        m_buttonIdToDelete = button.id;
                        m_showDeleteConfirmation = true; // Trigger the modal popup
                    }
                    ImGui::PopID(); // Pop ID
                }
                ImGui::EndTable();
            }
        }
    }

    DrawDeleteConfirmationModal();
}

// <<< ADDED: Implementation for the new public method >>>
#ifdef _WIN32
void ButtonListComponent::ProcessDroppedFiles(const std::vector<std::wstring>& files) {
    std::cout << "[ButtonList] Processing " << files.size() << " dropped files (Windows) passed from UIManager." << std::endl;
    for (const std::wstring& filePathW : files) {
        ProcessDroppedFile(filePathW); // Call the existing helper for single files
    }
}
#else
void ButtonListComponent::ProcessDroppedFiles(const std::string>& files) {
    std::cout << "[ButtonList] Processing " << files.size() << " dropped files passed from UIManager." << std::endl;
    for (const std::string& filePath : files) {
        ProcessDroppedFile(filePath); // Call the existing helper for single files
    }
}
#endif

// <<< Modify the private ProcessDroppedFile helper >>>
#ifdef _WIN32
void ButtonListComponent::ProcessDroppedFile(const std::wstring& filePathW) {
    std::cout << "[ButtonList DEBUG] Processing wstring path: " << WideToUtf8(filePathW) << std::endl;
    std::wcout << L"[ButtonList DEBUG] Processing wstring path (raw hex):" << WStringToHex(filePathW) << std::endl;

    // <<< DEBUG: Check existence using Windows API directly >>>
    if (_waccess(filePathW.c_str(), 0) == 0) {
        std::cout << "[ButtonList DEBUG] _waccess succeeded for: " << WideToUtf8(filePathW) << std::endl;
    } else {
        std::cerr << "[ButtonList DEBUG] _waccess FAILED for: " << WideToUtf8(filePathW) << " (errno: " << errno << ")" << std::endl;
    }

    try {
        fs::path p(filePathW);
        if (!fs::exists(p)) {
            std::cerr << "[ButtonList] Dropped file does not exist (reported by fs::exists): " << WideToUtf8(filePathW) << std::endl;
            return;
        }

        std::wstring wExtension = p.has_extension() ? p.extension().wstring() : L"";
        std::wstring wStem = p.stem().wstring();
        std::string extension = WideToUtf8(wExtension);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::string stem = WideToUtf8(wStem);

        PrefilledButtonData data;
        data.suggested_id = "btn_" + stem;
        std::replace_if(data.suggested_id.begin(), data.suggested_id.end(), [](char c){ return !std::isalnum(c) && c != '_'; }, '_');
        data.suggested_name = stem;

        // --- Action Type Detection ---
        std::string actionParamUtf8 = WideToUtf8(filePathW);

        if (extension == ".exe" || extension == ".bat" || extension == ".sh" || extension == ".app") {
            data.action_type = "launch_app";
            data.action_param = actionParamUtf8; 

            // <<< ADDED: Attempt to extract icon for .exe >>>
            if (extension == ".exe") {
                std::string outputDir = "assets/icons";
                auto extractedIconPathOpt = IconUtils::ExtractAndSaveIconPng(filePathW, outputDir, stem);
                if (extractedIconPathOpt) {
                    data.suggested_icon_path = *extractedIconPathOpt;
                     std::cout << "[ButtonList] Successfully extracted and set icon path: " << data.suggested_icon_path << std::endl;
                } else {
                    std::cerr << "[ButtonList] Failed to extract icon for " << actionParamUtf8 << std::endl;
                    // Keep suggested_icon_path empty or default
                }
            }
            // <<< END ADDED >>>

        } else if (extension == ".url") {
            data.action_type = "open_url";
            data.action_param = ""; // Placeholder
            std::cerr << "[ButtonList] .url parsing not implemented yet." << std::endl;
        } else if (extension == ".lnk") {
            data.action_type = "launch_app";
            data.action_param = actionParamUtf8; 
            // <<< MODIFIED: Use original stem for LNK placeholder too >>>
            std::cerr << "[ButtonList] .lnk file icon extraction via IconUtils not implemented yet." << std::endl;
            /* // Example of future LNK handling:
            std::string outputDir = "assets/icons";
            auto extractedIconPathOpt = IconUtils::ExtractAndSaveIconPng(filePathW, outputDir, stem);
            if (extractedIconPathOpt) {
                data.suggested_icon_path = *extractedIconPathOpt;
                 std::cout << "[ButtonList] Successfully extracted and set icon path from LNK: " << data.suggested_icon_path << std::endl;
            } else {
                 std::cerr << "[ButtonList] Failed to extract icon for LNK " << actionParamUtf8 << std::endl;
            }
            */
            // <<< END ADDED >>>
        } else {
            std::cout << "[ButtonList] Dropped file type '" << extension << "' not recognized for automatic action." << std::endl;
            data.action_type = "launch_app";
            data.action_param = actionParamUtf8;
        }

        // --- Icon Path (Only set if not already set by extractor) ---
        if (data.suggested_icon_path.empty() && 
            (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".gif")) 
        {
             data.suggested_icon_path = actionParamUtf8; 
        }

        // Trigger callback
        if (!data.action_type.empty() && m_onAddRequested) {
             std::cout << "[ButtonList] Requesting add for button: ID='" << data.suggested_id
                       << ", Name='" << data.suggested_name << "', Type='" << data.action_type << "', Param='"
                       << data.action_param << "', Icon='" << data.suggested_icon_path << "'" << std::endl;
            m_onAddRequested(data);
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ButtonList] Filesystem error processing dropped file: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ButtonList] Error processing dropped file: " << e.what() << std::endl;
    }
}
#else
// Non-Windows implementation (assumes UTF-8 from the start)
void ButtonListComponent::ProcessDroppedFile(const std::string& filePath) {
    try {
        fs::path p(filePath); // Assume filePath is UTF-8
        if (!fs::exists(p)) {
            std::cerr << "[ButtonList] Dropped file does not exist: " << filePath << std::endl;
            return;
        }

        std::string extension = p.has_extension() ? p.extension().string() : "";
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::string stem = p.stem().string();

        PrefilledButtonData data;
        data.suggested_id = "btn_" + stem;
        std::replace_if(data.suggested_id.begin(), data.suggested_id.end(), [](char c){ return !std::isalnum(c) && c != '_'; }, '_');
        data.suggested_name = stem;

        // --- Action Type Detection ---
        if (extension == ".exe" || extension == ".bat" || extension == ".sh" || extension == ".app") {
            data.action_type = "launch_app";
            data.action_param = filePath;
        } else if (extension == ".url") {
            data.action_type = "open_url";
            data.action_param = ""; // Placeholder
            std::cerr << "[ButtonList] .url parsing not implemented yet." << std::endl;
        } // .lnk check is implicitly Windows only
        else {
            std::cout << "[ButtonList] Dropped file type '" << extension << "' not recognized for automatic action." << std::endl;
            data.action_type = "launch_app";
            data.action_param = filePath;
        }

        // --- Icon Path ---
        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".gif") {
             data.suggested_icon_path = filePath;
        }

        // Trigger callback
        if (!data.action_type.empty() && m_onAddRequested) {
             std::cout << "[ButtonList] Requesting add for button: ID='" << data.suggested_id
                       << ", Name='" << data.suggested_name << "', Type='" << data.action_type << "', Param='"
                       << data.action_param << "', Icon='" << data.suggested_icon_path << "'" << std::endl;
            m_onAddRequested(data);
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ButtonList] Filesystem error processing dropped file: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ButtonList] Error processing dropped file: " << e.what() << std::endl;
    }
}
#endif

void ButtonListComponent::DrawDeleteConfirmationModal() {
    if (m_showDeleteConfirmation) {
        ImGui::OpenPopup(m_translator.get("delete_confirm_title").c_str());
        m_showDeleteConfirmation = false; // Reset flag after opening
    }

    // Standard modal structure
    if (ImGui::BeginPopupModal(m_translator.get("delete_confirm_title").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(m_translator.get("delete_confirm_text").c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s", m_buttonIdToDelete.c_str());
        ImGui::Separator();

        bool shouldClosePopup = false;
        bool configChanged = false;

        if (ImGui::Button(m_translator.get("delete_confirm_yes").c_str(), ImVec2(120, 0))) {
            if (m_configManager.removeButton(m_buttonIdToDelete)) {
                 std::cout << m_translator.get("button_removed_log") << m_buttonIdToDelete << std::endl;
                 // Attempt to save immediately after removing
                 if (m_configManager.saveConfig()) {
                     std::cout << m_translator.get("config_saved_delete_log") << std::endl;
                     configChanged = true; // Indicate config changed and saved
                 } else {
                     std::cerr << m_translator.get("config_save_fail_delete_log") << std::endl;
                     // TODO: Provide user feedback in UI about save failure?
                 }
            } else {
                 std::cerr << m_translator.get("remove_button_fail_log") << m_buttonIdToDelete
                           << m_translator.get("remove_button_fail_log_suffix") << std::endl;
                 // TODO: Provide user feedback in UI about remove failure?
            }
            m_buttonIdToDelete = ""; // Clear ID after attempt
            shouldClosePopup = true;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(m_translator.get("delete_confirm_cancel").c_str(), ImVec2(120, 0))) {
            std::cout << m_translator.get("delete_cancel_log") << m_buttonIdToDelete << std::endl;
            m_buttonIdToDelete = "";
            shouldClosePopup = true;
        }

        if (shouldClosePopup) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();

        // Note: If deletion happened, the list will redraw automatically on the next frame
        // and the deleted button will be gone. No explicit refresh needed here.
        // However, if the parent window (UIConfigurationWindow) has related state
        // (like the edit form showing the deleted button), the parent needs to handle that.
        // The current callback mechanism doesn't directly address clearing the edit form
        // if the edited item is deleted. This might need refinement later.
    }
}