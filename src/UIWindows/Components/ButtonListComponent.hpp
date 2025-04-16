#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional> // For std::function (callback)
#include "../../Managers/ConfigManager.hpp"
#include "../../Managers/TranslationManager.hpp"

class ButtonListComponent {
public:
    // Callback type: void(const std::string& buttonId)
    using EditRequestCallback = std::function<void(const std::string&)>;

    ButtonListComponent(ConfigManager& configManager, TranslationManager& translator, EditRequestCallback onEditRequested);

    void Draw();

private:
    ConfigManager& m_configManager;
    TranslationManager& m_translator;
    EditRequestCallback m_onEditRequested; // Callback to notify parent

    // State for delete confirmation
    std::string m_buttonIdToDelete = "";
    bool m_showDeleteConfirmation = false;

    void DrawDeleteConfirmationModal(); // Helper for the modal
};
