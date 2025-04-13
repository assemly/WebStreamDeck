#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click

UIManager::UIManager()
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

    const float button_size = 100.0f;
    const int buttons_per_row = 4;
    int button_count = 0;

    for (int i = 0; i < 12; ++i) // Example: Create 12 buttons
    {
        ImGui::PushID(i);
        char label[32];
        sprintf(label, "Button %d", i + 1);

        if (ImGui::Button(label, ImVec2(button_size, button_size)))
        {
            printf("Button %d clicked!\n", i + 1);
        }

        button_count++;
        if (button_count % buttons_per_row != 0)
        {
            ImGui::SameLine();
        }
        else
        {
            button_count = 0;
        }
        ImGui::PopID();
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