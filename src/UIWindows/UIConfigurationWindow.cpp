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
      m_translator(translationManager),
      m_presetManagerComponent(configManager, translationManager, uiManager),
      m_buttonListComponent(configManager, translationManager,
                          [this](const std::string& id){ this->HandleEditRequest(id); },
                          [this](const PrefilledButtonData& data){ this->HandleAddRequest(data); }),
      m_buttonEditComponent(configManager, translationManager)
{
    loadCurrentSettings(); // Load initial layout settings
}

void UIConfigurationWindow::HandleEditRequest(const std::string& buttonId) {
    std::cout << "[UIConfigWindow] Received edit request for ID: " << buttonId << std::endl;
    auto buttonOpt = m_configManager.getButtonById(buttonId);
    if (buttonOpt) {
        m_buttonEditComponent.StartEdit(*buttonOpt); // Call component's edit method
    } else {
        std::cerr << "[UIConfigWindow] Error: Button with ID " << buttonId << " not found for editing." << std::endl;
    }
}

void UIConfigurationWindow::HandleAddRequest(const PrefilledButtonData& data) {
    std::cout << "[UIConfigWindow] Received add request from drop. Suggested ID: " << data.suggested_id << std::endl;
    if (m_configManager.getButtonById(data.suggested_id)) {
         std::cerr << "[UIConfigWindow] Warning: Suggested ID '" << data.suggested_id << "' already exists. User must change it." << std::endl;
    }
    m_buttonEditComponent.StartAddNewPrefilled(data);
}

void UIConfigurationWindow::loadCurrentSettings() {
    const auto& layout = m_configManager.getLayoutConfig();
    m_tempPageCount = layout.page_count;
    m_tempRowsPerPage = layout.rows_per_page;
    m_tempColsPerPage = layout.cols_per_page;
}

void UIConfigurationWindow::Draw() {
    ImGui::Begin(m_translator.get("config_window_title").c_str());

    m_buttonListComponent.Draw();

    ImGui::Separator();

    m_buttonEditComponent.Draw();

    m_presetManagerComponent.Draw();

    ImGui::End();
}