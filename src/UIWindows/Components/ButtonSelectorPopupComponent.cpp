#include "ButtonSelectorPopupComponent.hpp"
#include "../../Managers/ConfigManager.hpp" // Included via header, but good practice
#include "../../Managers/TranslationManager.hpp" // Included via header, but good practice
#include <algorithm> // For std::transform, std::max
#include <iostream>  // For logging (optional)

ButtonSelectorPopupComponent::ButtonSelectorPopupComponent(ConfigManager& configManager, TranslationManager& translator)
    : m_configManager(configManager), m_translator(translator) {}

void ButtonSelectorPopupComponent::OpenPopup(int targetPage, int targetRow, int targetCol) {
    m_shouldOpen = true; // Set flag to request opening on the next Draw call
    m_targetPage = targetPage;
    m_targetRow = targetRow;
    m_targetCol = targetCol;
    m_filterBuffer[0] = '\0'; // Clear filter when opening
    // ImGui::OpenPopup is called within Draw to ensure it happens in a valid context
}

void ButtonSelectorPopupComponent::Draw(SelectionCallback onSelected) {
    // If open was requested, tell ImGui to open the popup
    if (m_shouldOpen) {
        ImGui::OpenPopup(m_popupId);
        m_shouldOpen = false; // Reset the request flag; ImGui manages the open state now
    }

    // Set position and size for the popup when it appears
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Appearing);

    // Begin the modal popup logic
    if (ImGui::BeginPopupModal(m_popupId, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        // Description text
        // Make sure the translation key exists and accepts arguments
        ImGui::Text(m_translator.get("select_button_popup_desc").c_str(), m_targetPage, m_targetRow, m_targetCol);
        ImGui::Separator();

        // Filter Input
        ImGui::PushItemWidth(-1); // Use full width
        ImGui::InputTextWithHint("##SelectButtonFilterPopup", m_translator.get("filter_placeholder").c_str(), m_filterBuffer, sizeof(m_filterBuffer));
        ImGui::PopItemWidth();
        std::string filterStrLower = m_filterBuffer;
        std::transform(filterStrLower.begin(), filterStrLower.end(), filterStrLower.begin(), ::tolower);

        ImGui::Separator();

        // Button List Area
        float listHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 2; // Reserve space for buttons
        if (ImGui::BeginChild("ButtonSelectionListPopup", ImVec2(0, std::max(listHeight, 100.0f)), true, ImGuiWindowFlags_HorizontalScrollbar)) {
            const auto& allButtons = m_configManager.getButtons();
            bool selectionMade = false;

            for (const ButtonConfig& button : allButtons) {
                // Apply filter (case-insensitive on name and ID)
                std::string buttonNameLower = button.name;
                std::string buttonIdLower = button.id;
                std::transform(buttonNameLower.begin(), buttonNameLower.end(), buttonNameLower.begin(), ::tolower);
                std::transform(buttonIdLower.begin(), buttonIdLower.end(), buttonIdLower.begin(), ::tolower);

                if (filterStrLower.empty() ||
                    buttonNameLower.find(filterStrLower) != std::string::npos ||
                    buttonIdLower.find(filterStrLower) != std::string::npos)
                {
                    // Display format: "Button Name (ID: button_id)"
                    std::string selectableLabel = button.name + " (ID: " + button.id + ")";
                    if (ImGui::Selectable(selectableLabel.c_str())) {
                        std::cout << "[ButtonSelectorPopup] Selected button '" << button.id << "' for position ["
                                  << m_targetPage << "," << m_targetRow << "," << m_targetCol << "]" << std::endl;

                        // Execute the callback provided by the caller
                        if (onSelected) {
                            onSelected(button.id, m_targetPage, m_targetRow, m_targetCol);
                        }

                        selectionMade = true;
                        ImGui::CloseCurrentPopup(); // Close popup after selection
                    }
                }
            }
            // Must end child before checking selectionMade and potentially returning
            ImGui::EndChild();

            // If a selection was made, close the popup and exit the Draw function early for this frame
            if (selectionMade) {
                 ImGui::EndPopup();
                 return;
            }

        } else {
            // EndChild should still be called if BeginChild returns false
             ImGui::EndChild();
        }

        ImGui::Separator();

        // Cancel Button
        if (ImGui::Button(m_translator.get("cancel_button_label").c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup(); // End the modal frame
    }
    // If BeginPopupModal returns false, the popup is not open/visible this frame.
} 