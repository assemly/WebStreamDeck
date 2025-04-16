#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional> // For callback
#include "../../Managers/ConfigManager.hpp" // Needs ButtonConfig definition
#include "../../Managers/TranslationManager.hpp" // Needs TranslationManager definition

// Forward declarations are generally preferred in headers if full definition isn't needed
// However, we likely need the definitions for the constructor parameters.
// class ConfigManager;
// class TranslationManager;

class ButtonSelectorPopupComponent {
public:
    // Callback type: Notifies when a button is selected.
    // Parameters: selected button ID, target page, target row, target column
    using SelectionCallback = std::function<void(const std::string&, int, int, int)>;

    ButtonSelectorPopupComponent(ConfigManager& configManager, TranslationManager& translator);

    // Call this to request the popup to open for a specific target cell
    void OpenPopup(int targetPage, int targetRow, int targetCol);

    // Call this every frame. It handles drawing the popup if it's open
    // and calls the provided callback upon successful selection.
    void Draw(SelectionCallback onSelected);

private:
    ConfigManager& m_configManager;
    TranslationManager& m_translator;

    // Internal state
    bool m_shouldOpen = false; // Flag to control if the popup should be requested to open
    int m_targetPage = -1;
    int m_targetRow = -1;
    int m_targetCol = -1;
    char m_filterBuffer[128] = {0};
    const char* m_popupId = "SelectExistingButtonPopupID"; // Unique ID for the popup modal
}; 