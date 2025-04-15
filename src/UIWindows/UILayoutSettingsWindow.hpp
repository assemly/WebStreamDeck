#pragma once

#include <imgui.h>
#include <string>
// #include <vector> // Not strictly needed in this header based on current content

// Forward declarations
class ConfigManager;
class TranslationManager;
class UIManager;

class UILayoutSettingsWindow {
public:
    // Constructor needs ConfigManager, Translator, and UIManager
    UILayoutSettingsWindow(ConfigManager& configManager, TranslationManager& translationManager, UIManager& uiManager);
    // Removed destructor, add back if needed

    // Main drawing function for the window
    void Draw();

    // Removed Open(), Close(), IsOpen()

private:
    ConfigManager& m_configManager;
    TranslationManager& m_translator;
    UIManager& m_uiManager; // To notify layout changes

    // Removed m_isOpen

    // Temporary storage for UI input values (renamed for consistency with my earlier suggestion)
    int m_editedRowsPerPage = 0;
    int m_editedColsPerPage = 0;
    int m_editedPageCount = 0;

    // Flag to load initial values only once or when needed
    bool m_isInitialized = false; 

    // Helper to load current settings into temporary variables
    void loadCurrentSettings();

    // <<< ADDED: Helper to apply the edited settings >>>
    void applyLayoutChanges(); 
};