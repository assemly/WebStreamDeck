#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click

UIManager::UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor)
    : m_configManager(configManager),
      m_actionExecutor(actionExecutor)
{
    // Constructor: Initialize any UI state if needed
}

UIManager::~UIManager()
{
    // Destructor: Clean up any resources if needed
}

void UIManager::drawUI()
{
    // Main UI drawing function called from the main loop

    // Enable docking over the main viewport
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // Draw individual UI windows
    drawButtonGridWindow();
    drawConfigurationWindow();
    drawStatusLogWindow();
    drawQrCodeWindow();
}

void UIManager::drawButtonGridWindow()
{
    ImGui::Begin("Button Grid");

    const std::vector<ButtonConfig>& buttons = m_configManager.getButtons();

    if (buttons.empty()) {
        ImGui::TextUnformatted("No buttons configured. Add buttons in config.json or via Configuration panel.");
    } else {
        const float button_size = 100.0f;
        const int buttons_per_row = 4; // You might want to make this dynamic based on window width later
        int button_count = 0;

        for (const auto& button : buttons)
        {
            ImGui::PushID(button.id.c_str()); // Use button ID for unique ImGui ID
            
            // Use button name as label, handle potential encoding issues if necessary
            if (ImGui::Button(button.name.c_str(), ImVec2(button_size, button_size)))
            {
                printf("Button '%s' (ID: %s) clicked! Action: %s(%s)\n", 
                       button.name.c_str(), button.id.c_str(), 
                       button.action_type.c_str(), button.action_param.c_str());
                // Trigger the actual action execution here
                m_actionExecutor.executeAction(button.id);
            }

            button_count++;
            if (button_count % buttons_per_row != 0)
            {
                ImGui::SameLine();
            }
            else
            {
                button_count = 0; // Reset for next row (or handle wrapping better)
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void UIManager::drawConfigurationWindow()
{
    ImGui::Begin("Configuration");
    // TODO: Implement configuration controls
    ImGui::Text("Button configuration options here.");
    ImGui::End();
}

void UIManager::drawStatusLogWindow()
{
    ImGui::Begin("Status/Log");
    // TODO: Implement status and logging display
    ImGui::Text("Connection status and logs here.");
    ImGui::End();
}

void UIManager::drawQrCodeWindow()
{
    ImGui::Begin("QR Code");
    // TODO: Implement QR code display
    ImGui::Text("QR code for mobile connection here.");
    ImGui::End();
} 