// src/UIWindows/Components/ButtonListComponent.cpp
#include "ButtonListComponent.hpp"
#include <imgui.h>
#include <iostream> // For std::cout, std::cerr

ButtonListComponent::ButtonListComponent(ConfigManager& configManager, TranslationManager& translator, EditRequestCallback onEditRequested)
    : m_configManager(configManager),
      m_translator(translator),
      m_onEditRequested(std::move(onEditRequested)) // Store the callback
{}

void ButtonListComponent::Draw() {
    


    const auto& buttons = m_configManager.getButtons();

    // Use a CollapsingHeader to make the list expandable
    // std::string headerLabel = m_translator.get("loaded_buttons_header");
    // headerLabel += " (" + std::to_string(buttons.size()) + ")"; // Add count to header
    
    std::string visibleText = m_translator.get("loaded_buttons_header") + " (" + std::to_string(buttons.size()) + ")";
    std::string fullLabel = visibleText + "##LoadedButtonsHeader"; // "##" 后面是稳定 ID

    // Set ImGuiTreeNodeFlags for the header

    if (ImGui::CollapsingHeader(fullLabel.c_str(),  ImGuiTreeNodeFlags_DefaultOpen)) {

        // --- Loaded Buttons Table (only drawn if header is open) ---
        if (buttons.empty()) {
            ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
        } else {
            // Removed the separate ImGui::TextUnformatted for the header here as it's now part of CollapsingHeader
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
    } // End of CollapsingHeader block
    else {
    
    }

    // Draw the modal if triggered (outside the collapsing header)
    DrawDeleteConfirmationModal();


}


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