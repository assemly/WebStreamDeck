#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr
#include <qrcodegen.hpp> // Re-add QR Code generation library


// Define STB_IMAGE_IMPLEMENTATION in *one* CPP file before including stb_image.h
// #define STB_IMAGE_IMPLEMENTATION // <<< REMOVED: Now defined in UIButtonGridWindow.cpp
#include <stb_image.h>       // Include stb_image header for image loading

#pragma comment(lib, "Ws2_32.lib") // Link against Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib") // Link against Iphlpapi.lib

// Helper to convert QR Code data to an OpenGL texture
// GLuint qrCodeToTextureHelper(const qrcodegen::QrCode& qr) { // <<< REMOVED: Moved to UIQrCodeWindow
    // ... (implementation removed)
// }

// --- ADDED: Implementation for loading texture from file ---
// GLuint UIManager::LoadTextureFromFile(const char* filename) { // <<< REMOVED: Moved to UIButtonGridWindow
//     int width, height, channels;
//     // Force 4 channels (RGBA) for consistency with OpenGL formats
//     unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
//     if (data == nullptr) {
//         std::cerr << "Error loading image: " << filename << " - " << stbi_failure_reason() << std::endl;
//         return 0; // Return 0 indicates failure
//     }
// 
//     GLuint textureID;
//     glGenTextures(1, &textureID);
//     glBindTexture(GL_TEXTURE_2D, textureID);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
// #if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
// #endif
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//     stbi_image_free(data);
//     glBindTexture(GL_TEXTURE_2D, 0);
//     std::cout << "Loaded texture: " << filename << " (ID: " << textureID << ")" << std::endl;
//     return textureID;
// }

// --- End of LoadTextureFromFile --- 

// --- ADDED: Implementation for releasing animated GIF textures ---
// void UIManager::releaseAnimatedGifTextures() { // <<< REMOVED: Moved to UIButtonGridWindow
//     for (auto const& [path, gifData] : m_animatedGifTextures) {
//         if (gifData.loaded) {
//             glDeleteTextures(gifData.frameTextureIds.size(), gifData.frameTextureIds.data());
//              std::cout << "Deleted " << gifData.frameTextureIds.size() << " GIF textures for: " << path << std::endl;
//         }
//     }
//     m_animatedGifTextures.clear();
// }
// --- End of releaseAnimatedGifTextures --- 

UIManager::UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager)
    : m_configManager(configManager),
      m_actionExecutor(actionExecutor),
      m_translator(translationManager),
      m_buttonGridWindow(configManager, actionExecutor, translationManager),
      m_configWindow(configManager, translationManager),
      m_statusLogWindow(translationManager),
      m_qrCodeWindow(translationManager) // <<< ADDED: Initialize the QR code window
{
    m_serverIP = NetworkUtils::GetLocalIPv4(); // <<< ADDED: Call helper on startup
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

    // drawButtonGridWindow(); // <<< REMOVED
    m_buttonGridWindow.Draw(); // <<< ADDED: Call the new class's Draw method
    m_configWindow.Draw(); // <<< ADDED: Call the new class's Draw method
    m_statusLogWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
    // drawQrCodeWindow(); // <<< REMOVED
    m_qrCodeWindow.Draw(m_isServerRunning, m_serverPort, m_serverIP, [this]() {
        m_serverIP = NetworkUtils::GetLocalIPv4();
    });
}


// Implementation of the new method
void UIManager::setServerStatus(bool isRunning, int port) {
    m_isServerRunning = isRunning;
    m_serverPort = port;
}

// Helper function to get the local IPv4 address (Windows specific)
// void UIManager::updateLocalIP() { // <<< REMOVED: Entire function moved to NetworkUtils
    // ... (implementation removed)
// }

// Helper function to get the local IPv4 address (Windows specific)
// void UIManager::updateLocalIP() { // <<< REMOVED: Entire function moved to NetworkUtils
    // ... (implementation removed)
// } 