#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include "ActionRequestManager.hpp"
#include "../Utils/NetworkUtils.hpp"
#include "../Services/WebSocketServer.hpp" // Ensure included for implementation
#include "../UIWindows/UILayoutSettingsWindow.hpp" // <<< ADDED: Include new window header
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr, std::cout
#include <qrcodegen.hpp> // Re-add QR Code generation library
#include <string>


// Define STB_IMAGE_IMPLEMENTATION in *one* CPP file before including stb_image.h
// #define STB_IMAGE_IMPLEMENTATION // <<< REMOVED: Now defined in UIButtonGridWindow.cpp
#include <stb_image.h>       // Include stb_image header for image loading

#pragma comment(lib, "Ws2_32.lib") // Link against Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib") // Link against Iphlpapi.lib


// <<< MODIFIED: Constructor takes NetworkManager reference and initializes new window >>>
UIManager::UIManager(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, NetworkManager& networkManager)
      : m_buttonGridWindow(configManager, actionRequestManager, translationManager, *this),
      m_configWindow(*this, configManager, translationManager),
      m_statusLogWindow(translationManager),
      m_qrCodeWindow(translationManager),
      m_layoutSettingsWindow(configManager, translationManager, *this), // <<< ADDED: Initialize new window
      m_networkManager(networkManager), // Initialize NetworkManager reference
      m_translationManager(translationManager) // <<< ADDED: Initialize TranslationManager reference
{
    m_serverIP = NetworkUtils::GetLocalIPv4();
}

// --- MODIFIED: Destructor --- 
UIManager::~UIManager()
{
    // No cleanup needed here currently for UIManager itself
    // TextureLoader::ReleaseStaticTextures(); // <<< REMOVED: Moved to main.cpp
}

void UIManager::drawUI()
{
    // <<< ADDED: Create the main DockSpace >>>
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; // Dockspace should not be dockable itself
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    // if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) // We are not using PassthruCentralNode here
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3); // Pop WindowRounding, WindowBorderSize, WindowPadding

    // Submit the DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None); // Removed PassthruCentralNode

    // <<< MOVED: Draw windows *after* setting up the DockSpace >>>
    m_buttonGridWindow.Draw();
    m_configWindow.Draw();
    m_statusLogWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
    m_qrCodeWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
    m_layoutSettingsWindow.Draw(); // <<< Draw the layout settings window

    // <<< ADDED: End the DockSpace window >>>
    ImGui::End(); 

    // <<< ADDED: Set initial focus on the first frame >>>
    if (m_firstFrame) {
        // Assuming the button grid window title is obtained via the translator
        const char* gridWindowTitle = m_translationManager.get("button_grid_window_title").c_str();
        ImGui::SetWindowFocus(gridWindowTitle);
        m_firstFrame = false; // Only do this once
        std::cout << "[UIManager] Set initial focus to: " << gridWindowTitle << std::endl;
    }
}


// Implementation of the new method
void UIManager::setServerStatus(bool isRunning, int port) {
    m_isServerRunning = isRunning;
    m_serverPort = port;
}

// <<< MODIFIED: Implementation for notifyLayoutChanged >>>
void UIManager::notifyLayoutChanged() {
    m_buttonGridWindow.onLayoutChanged();
    m_configWindow.onLayoutChanged(); // Notify config window too if needed
    m_networkManager.broadcastWebSocketState();
}

// <<< ADDED: Implementation for processing dropped files >>>
#ifdef _WIN32
void UIManager::ProcessDroppedFiles(const std::vector<std::wstring>& files) {
    // Forward the call to the relevant UI component
    // Assuming UIConfigurationWindow holds or can access ButtonListComponent
    // And UIConfigurationWindow has a matching ProcessDroppedFiles method
    m_configWindow.ProcessDroppedFiles(files);
}
#else
void UIManager::ProcessDroppedFiles(const std::vector<std::string>& files) {
    // Forward the call
    m_configWindow.ProcessDroppedFiles(files);
}
#endif
