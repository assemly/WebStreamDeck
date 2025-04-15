#include "UILayoutSettingsWindow.hpp"
#include "../Managers/ConfigManager.hpp" // Include necessary headers
#include "../Managers/TranslationManager.hpp"
#include "../Managers/UIManager.hpp"
#include <iostream> // For logging

// Constructor implementation
UILayoutSettingsWindow::UILayoutSettingsWindow(ConfigManager& configManager, TranslationManager& translationManager, UIManager& uiManager)
    : m_configManager(configManager),
      m_translator(translationManager),
      m_uiManager(uiManager) // Initialize references
{
    // Initialization moved to Draw() using m_isInitialized flag
}

// Helper function to load current settings into editable UI variables
void UILayoutSettingsWindow::loadCurrentSettings() {
    const auto& currentLayout = m_configManager.getLayoutConfig();
    m_editedRowsPerPage = currentLayout.rows_per_page;
    m_editedColsPerPage = currentLayout.cols_per_page;
    m_editedPageCount = currentLayout.page_count;
    std::cout << "[LayoutSettings] Loaded current settings: Pages=" << m_editedPageCount 
              << ", Rows=" << m_editedRowsPerPage << ", Cols=" << m_editedColsPerPage << std::endl;
}

// Helper function to apply edited settings
void UILayoutSettingsWindow::applyLayoutChanges() {
    std::cout << "[LayoutSettings] Applying layout changes: Pages=" << m_editedPageCount 
              << ", Rows=" << m_editedRowsPerPage << ", Cols=" << m_editedColsPerPage << std::endl;

    // <<< MODIFIED: Call the actual ConfigManager method >>>
    bool success = m_configManager.setLayoutDimensions(m_editedPageCount, m_editedRowsPerPage, m_editedColsPerPage);
    // bool success = false; // Placeholder removed

    if (success) {
        std::cout << "[LayoutSettings] ConfigManager updated successfully. Notifying UIManager." << std::endl;
        m_uiManager.notifyLayoutChanged(); // Notify other UI parts and save config
        // <<< ADDED: Reload current settings into UI after successful save >>>
        // This ensures UI reflects the potentially adjusted values (e.g., if page count was invalid)
        // Although setLayoutDimensions should handle validation, it's good practice.
        loadCurrentSettings(); 
    } else {
        std::cerr << "[LayoutSettings] Failed to apply layout changes via ConfigManager! Reverting UI edits." << std::endl;
        // Revert UI fields if saving failed
        loadCurrentSettings(); 
        // Optionally show an error message in the UI
    }
}

// Main Draw function
void UILayoutSettingsWindow::Draw() {
    // <<< REMOVED: m_isOpen check >>>

    // <<< ADDED: Initialization check >>>
    if (!m_isInitialized) {
        loadCurrentSettings();
        m_isInitialized = true;
    }

    // Start the ImGui window
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    // <<< MODIFIED: Removed &m_isOpen parameter >>>
    if (ImGui::Begin(m_translator.get("layout_settings_window_title").c_str())) { // Need translation key

        ImGui::TextWrapped("%s", m_translator.get("layout_settings_description").c_str()); // Need translation key
        ImGui::Separator();

        // --- Input fields --- 
        bool valueChanged = false;
        // Use InputInt with minimum value constraints
        ImGui::PushItemWidth(100); // Adjust width as needed
        valueChanged |= ImGui::InputInt(m_translator.get("page_count_label").c_str(), &m_editedPageCount); // Need translation key
        if (m_editedPageCount < 1) m_editedPageCount = 1;
        valueChanged |= ImGui::InputInt(m_translator.get("rows_per_page_label").c_str(), &m_editedRowsPerPage); // Need translation key
        if (m_editedRowsPerPage < 1) m_editedRowsPerPage = 1;
        valueChanged |= ImGui::InputInt(m_translator.get("cols_per_page_label").c_str(), &m_editedColsPerPage); // Need translation key
        if (m_editedColsPerPage < 1) m_editedColsPerPage = 1;
        ImGui::PopItemWidth();

        // --- Action buttons --- 
        ImGui::Separator();
        // Apply button
        if (ImGui::Button(m_translator.get("apply_changes_button").c_str())) { // Need translation key
            applyLayoutChanges(); // Call the apply function
        }
        ImGui::SameLine();
        // Cancel/Reset button
        if (ImGui::Button(m_translator.get("reset_button_label").c_str())) { // Need translation key (or reuse cancel)
            loadCurrentSettings(); // Revert temporary variables to current settings
        }

    }
    ImGui::End(); // End the ImGui window
}
