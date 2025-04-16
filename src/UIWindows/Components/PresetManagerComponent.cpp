// src/UIWindows/Components/PresetManagerComponent.cpp
#include "PresetManagerComponent.hpp"
#include "../../Managers/ConfigManager.hpp"      // Need full definition
#include "../../Managers/TranslationManager.hpp" // Need full definition
#include "../../Managers/UIManager.hpp"          // Need full definition
#include "../../Utils/StringUtils.hpp"         // <<< ADDED


PresetManagerComponent::PresetManagerComponent(ConfigManager& configManager, TranslationManager& translationManager, UIManager& uiManager)
    : m_configManager(configManager),
      m_translator(translationManager),
      m_uiManager(uiManager)
{
    scanPresetDirectory(); // Scan for presets when the component is created
}

void PresetManagerComponent::scanPresetDirectory() {
    m_presetFileNames.clear();
    m_selectedPresetIndex = -1; // Reset selection

    try {
        if (!fs::exists(m_presetsDir)) {
            std::cout << "[PresetComponent] Preset directory not found: " << m_presetsDir << ". No presets loaded." << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(m_presetsDir)) {
            if (entry.is_regular_file()) {
                fs::path filepath = entry.path();
                std::string extension = filepath.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".json") {
                    // Get UTF-8 encoded filename and convert to std::string
                    std::u8string u8Name = filepath.stem().u8string();
                    m_presetFileNames.push_back(std::string(u8Name.begin(), u8Name.end()));
                }
            }
        }
        std::cout << "[PresetComponent] Scanned presets directory. Found " << m_presetFileNames.size() << " preset files." << std::endl;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[PresetComponent] Filesystem error while scanning presets directory: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[PresetComponent] Error scanning presets directory: " << e.what() << std::endl;
    }
}

void PresetManagerComponent::Draw() {
    ImGui::Separator();
    // ImGui::TextUnformatted(m_translator.get("preset_management_header").c_str()); // Remove separate header text

    // Wrap the entire component in a collapsing header, open by default
    if (ImGui::CollapsingHeader(m_translator.get("preset_management_header").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        // --- Preset Loading --- (inside the header)
        ImGui::TextUnformatted(m_translator.get("load_preset_label").c_str());
        ImGui::SameLine();

        std::vector<const char*> presetItems;
        for (const auto& name : m_presetFileNames) {
            presetItems.push_back(name.c_str());
        }

        ImGui::PushItemWidth(200);
        if (ImGui::Combo("##PresetSelect", &m_selectedPresetIndex, presetItems.data(), presetItems.size())) {
            // Selection changed - no immediate action needed here, handled by button
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (ImGui::Button(m_translator.get("load_selected_button").c_str())) {
            if (m_selectedPresetIndex >= 0 && m_selectedPresetIndex < m_presetFileNames.size()) {
                std::string selectedPresetName = m_presetFileNames[m_selectedPresetIndex];
                // Construct path using std::filesystem for Unicode safety on save,
                // but loading likely needs wide conversion on Windows too if preset name has non-ASCII.
                // Let's use the same wide string approach for consistency if on Windows.
                std::string fullPathStr;
                #ifdef _WIN32
                    std::wstring presetNameWide = StringUtils::Utf8ToWide(selectedPresetName); // Assumes StringUtils.hpp is included
                    if(!presetNameWide.empty()){
                        std::filesystem::path fullPath = std::filesystem::path(m_presetsDir) / (presetNameWide + L".json");
                        fullPathStr = fullPath.string(); // For logging
                        std::cout << "[PresetComponent] Attempting to load preset (wide path): " << fullPathStr << std::endl;
                         if (m_configManager.loadConfigFromFile(fullPath)) {
                             std::cout << "[PresetComponent] Preset loaded successfully. Notifying." << std::endl;
                             m_uiManager.notifyLayoutChanged(); // Notify UIManager
                             // Parent window might need update too (e.g., reload its temp settings)
                             // This component cannot directly call parent's private methods.
                             // Consider adding a return value or callback if parent needs update.
                         } else {
                             std::cerr << "[PresetComponent] Failed to load preset: " << fullPathStr << std::endl;
                         }
                    } else {
                         std::cerr << "[PresetComponent] Failed to convert selected preset name to wide string." << std::endl;
                    }
                #else
                     std::filesystem::path fullPath = std::filesystem::path(m_presetsDir) / (selectedPresetName + ".json");
                     fullPathStr = fullPath.string();
                     std::cout << "[PresetComponent] Attempting to load preset: " << fullPathStr << std::endl;
                     if (m_configManager.loadConfigFromFile(fullPath)) {
                         std::cout << "[PresetComponent] Preset loaded successfully. Notifying." << std::endl;
                         m_uiManager.notifyLayoutChanged();
                     } else {
                         std::cerr << "[PresetComponent] Failed to load preset: " << fullPathStr << std::endl;
                     }
                #endif
            } else {
                std::cout << "[PresetComponent] No preset selected or index out of bounds." << std::endl;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button(m_translator.get("refresh_list_button").c_str())) {
            scanPresetDirectory();
        }

        ImGui::Spacing();

        // --- Preset Saving --- (inside the header)
        ImGui::TextUnformatted(m_translator.get("save_preset_label").c_str());
        ImGui::SameLine();

        ImGui::PushItemWidth(200);
        ImGui::InputText("##NewPresetName", m_newPresetNameBuffer, sizeof(m_newPresetNameBuffer));
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (ImGui::Button(m_translator.get("save_as_button").c_str())) {
            std::string newName = m_newPresetNameBuffer;
            if (!newName.empty()) {
                 std::cout << "[PresetComponent] Attempting to save current config as preset: " << newName << std::endl;
                if (m_configManager.saveConfigToPreset(newName)) { // saveConfigToPreset already handles Unicode filenames
                    std::cout << "[PresetComponent] Preset saved successfully. Refreshing list." << std::endl;
                    scanPresetDirectory();
                    m_newPresetNameBuffer[0] = '\\0';
                } else {
                     std::cerr << "[PresetComponent] Failed to save preset: " << newName << std::endl;
                }
            } else {
                 std::cout << "[PresetComponent] Preset name cannot be empty." << std::endl;
            }
        }
    } // End of CollapsingHeader

    ImGui::Separator(); // Separator after the preset section (keep it outside or move inside? Let's keep it outside for visual structure)
}
