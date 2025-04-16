#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <map> // Although not directly used, ImGuiFileDialog might need it indirectly? Keep for safety or remove later.
#include "../Managers/ConfigManager.hpp"
#include "../Managers/TranslationManager.hpp"
#include <memory> // For UIManager access
#include "Components/PresetManagerComponent.hpp"
#include "Components/ButtonListComponent.hpp"
#include "Components/ButtonEditComponent.hpp"

// Forward declare ImGuiFileDialog if only pointers/references are used, or include header
// #include <ImGuiFileDialog.h>

// Forward declarations
class ConfigManager;
class UIManager;

class UIConfigurationWindow {
public:
    UIConfigurationWindow(UIManager& uiManager, ConfigManager& configManager, TranslationManager& translationManager);
    ~UIConfigurationWindow() = default; // Default destructor likely sufficient

    void Draw();
    bool IsVisible() const { return m_isVisible; }
    void Show() { m_isVisible = true; }
    void Hide() { m_isVisible = false; }

private:
    UIManager& m_uiManager;
    ConfigManager& m_configManager;
    TranslationManager& m_translator;

    // Component Instances
    PresetManagerComponent m_presetManagerComponent;
    ButtonListComponent m_buttonListComponent;
    ButtonEditComponent m_buttonEditComponent;

    // Callback handler for edit requests
    void HandleEditRequest(const std::string& buttonId);
    void HandleAddRequest(const PrefilledButtonData& data);

    // Temporary storage for editing layout settings
    int m_tempPageCount;
    int m_tempRowsPerPage;
    int m_tempColsPerPage;

    void loadCurrentSettings(); // Helper to load settings into temp vars

    bool m_isVisible = false;
};