#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include "ActionRequestManager.hpp"
#include "../Utils/NetworkUtils.hpp"
#include "../Services/WebSocketServer.hpp" // Ensure included for implementation
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr, std::cout
#include <qrcodegen.hpp> // Re-add QR Code generation library


// Define STB_IMAGE_IMPLEMENTATION in *one* CPP file before including stb_image.h
// #define STB_IMAGE_IMPLEMENTATION // <<< REMOVED: Now defined in UIButtonGridWindow.cpp
#include <stb_image.h>       // Include stb_image header for image loading

#pragma comment(lib, "Ws2_32.lib") // Link against Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib") // Link against Iphlpapi.lib


// <<< MODIFIED: Constructor takes NetworkManager reference >>>
UIManager::UIManager(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, NetworkManager& networkManager)
      : m_buttonGridWindow(configManager, actionRequestManager, translationManager, *this),
      m_configWindow(configManager, translationManager),
      m_statusLogWindow(translationManager),
      m_qrCodeWindow(translationManager),
      m_networkManager(networkManager) // Initialize NetworkManager reference
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
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    m_buttonGridWindow.Draw();
    m_configWindow.Draw();
    m_statusLogWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
    m_qrCodeWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
}


// Implementation of the new method
void UIManager::setServerStatus(bool isRunning, int port) {
    m_isServerRunning = isRunning;
    m_serverPort = port;
}

// <<< MODIFIED: Implementation for notifyLayoutChanged >>>
void UIManager::notifyLayoutChanged() {
    std::cout << "[UIManager] Layout changed, notifying NetworkManager to broadcast." << std::endl;
    m_networkManager.broadcastWebSocketState(); // Call the broadcast method
}
