#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr
#include <qrcodegen.hpp> // Re-add QR Code generation library

#pragma comment(lib, "Ws2_32.lib") // Link against Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib") // Link against Iphlpapi.lib

// Helper to convert QR Code data to an OpenGL texture
GLuint qrCodeToTextureHelper(const qrcodegen::QrCode& qr) {
    if (qr.getSize() == 0) return 0;
    int size = qr.getSize();
    int border = 1; // Add a small border
    int texture_size = size + 2 * border;
    std::vector<unsigned char> texture_data(texture_size * texture_size * 3); // RGB
    for (int y = 0; y < texture_size; ++y) {
        for (int x = 0; x < texture_size; ++x) {
            bool is_black = false;
            if (x >= border && x < size + border && y >= border && y < size + border) {
                is_black = qr.getModule(x - border, y - border);
            }
            unsigned char color = is_black ? 0 : 255;
            int index = (y * texture_size + x) * 3;
            texture_data[index + 0] = color;
            texture_data[index + 1] = color;
            texture_data[index + 2] = color;
        }
    }
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

UIManager::UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor)
    : m_configManager(configManager),
      m_actionExecutor(actionExecutor)
{
    updateLocalIP(); // Get IP on startup
}

UIManager::~UIManager()
{
    releaseQrTexture(); // Release texture in destructor
}

void UIManager::drawUI()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

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
    ImGui::Text("Button configuration options here.");
    ImGui::End();
}

void UIManager::drawStatusLogWindow()
{
    ImGui::Begin("Status/Log");
    ImGui::Text("Server Status: %s", m_isServerRunning ? "Running" : "Stopped");
    ImGui::Text("Server Address: ws://%s:%d", m_serverIP.c_str(), m_serverPort);
    if (ImGui::Button("Refresh IP")) {
        updateLocalIP();
    }
    ImGui::Separator();
    ImGui::Text("Logs here...");
    ImGui::End();
}

void UIManager::drawQrCodeWindow()
{
    ImGui::Begin("QR Code"); // Restore original name
    if (m_isServerRunning && m_serverPort > 0 && m_serverIP.find("Error") == std::string::npos && m_serverIP.find("Fetching") == std::string::npos && m_serverIP.find("No suitable") == std::string::npos) {
        std::string ws_address = "ws://" + m_serverIP + ":" + std::to_string(m_serverPort);

        // Generate QR texture only if the address has changed
        if (ws_address != m_lastGeneratedQrText) {
            generateQrTexture(ws_address);
        }

        ImGui::Text("Scan this QR code with your phone:");
        if (m_qrTextureId != 0) {
            ImGui::Image((ImTextureID)(intptr_t)m_qrTextureId, ImVec2(200, 200));
        } else {
            ImGui::Text("Failed to generate QR Code texture.");
        }
         ImGui::Text("Or connect to: %s", ws_address.c_str());
         if (ImGui::Button("Copy Address")) {
             ImGui::SetClipboardText(ws_address.c_str());
         }

    } else if (!m_isServerRunning) {
        ImGui::Text("Start the server to generate the QR Code.");
         if (m_qrTextureId != 0) { releaseQrTexture(); } // Release texture if server stops
    } else {
        ImGui::Text("Waiting for valid server IP address...");
        if (ImGui::Button("Retry Fetch IP")) {
            updateLocalIP();
        }
         if (m_qrTextureId != 0) { releaseQrTexture(); } // Release texture if IP is invalid
    }
    ImGui::End();
}

// Helper to generate/update QR Code texture
void UIManager::generateQrTexture(const std::string& text) {
    releaseQrTexture(); // Release existing texture first
    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        m_qrTextureId = qrCodeToTextureHelper(qr); // Use the helper
        if (m_qrTextureId != 0) {
            m_lastGeneratedQrText = text; // Store the text used for this texture
        }
         else {
             m_lastGeneratedQrText = ""; // Generation failed
         }
    } catch (const std::exception& e) {
        std::cerr << "Error generating QR Code: " << e.what() << std::endl;
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Helper to release the QR Code texture
void UIManager::releaseQrTexture() {
    if (m_qrTextureId != 0) {
        glDeleteTextures(1, &m_qrTextureId);
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Implementation of the new method
void UIManager::setServerStatus(bool isRunning, int port) {
    m_isServerRunning = isRunning;
    m_serverPort = port;
}

// Helper function to get the local IPv4 address (Windows specific)
void UIManager::updateLocalIP() {
    std::string oldIP = m_serverIP;
    m_serverIP = "Error fetching IP"; // Default error message
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // We want IPv4

    // Try to get the buffer size needed
    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (!pAddresses) {
        std::cerr << "Memory allocation failed for GetAdaptersAddresses\n";
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        if (!pAddresses) {
            std::cerr << "Memory allocation failed for GetAdaptersAddresses\n";
            return;
        }
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);
    }

    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            // Look for operational adapters (Up) that are not loopback and support IPv4
            if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
                pCurrAddresses->IfType != IF_TYPE_TUNNEL) // Exclude tunnels like VPNs
            {
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast) {
                    sockaddr* pAddr = pUnicast->Address.lpSockaddr;
                    if (pAddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(pAddr);
                        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
                        m_serverIP = ipStr; // Found a suitable IP
                        if (oldIP != m_serverIP) { // Regenerate QR if IP changed
                           releaseQrTexture(); // Force regeneration in drawQrCodeWindow
                        }
                        free(pAddresses);
                        return; // Use the first suitable IPv4 address found
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
        m_serverIP = "No suitable IP found";
    } else {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
    }

    if (pAddresses) {
        free(pAddresses);
    }
    if (oldIP != m_serverIP) { // Also release texture if IP becomes invalid
        releaseQrTexture();
    }
} 