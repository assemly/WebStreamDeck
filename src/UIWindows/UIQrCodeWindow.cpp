#include "UIQrCodeWindow.hpp"
#include <iostream> // For std::cerr, std::cout
#include <vector>   // For std::vector used in helper


UIQrCodeWindow::UIQrCodeWindow(TranslationManager& translationManager)
    : m_translator(translationManager) {}

UIQrCodeWindow::~UIQrCodeWindow() {
    releaseQrTexture(); // Ensure texture is released on destruction
}

// Helper to convert QR Code data to an OpenGL texture (Now a private member)
GLuint UIQrCodeWindow::qrCodeToTextureHelper(const qrcodegen::QrCode& qr) {
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
     if (textureID == 0) {
        std::cerr << "Error: Failed to generate texture ID (QRWindow)" << std::endl;
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}


// Helper to generate/update QR Code texture (Now a private member)
void UIQrCodeWindow::generateQrTexture(const std::string& text) {
    releaseQrTexture(); // Release existing texture first
    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        m_qrTextureId = qrCodeToTextureHelper(qr); // Use the member helper
        if (m_qrTextureId != 0) {
            m_lastGeneratedQrText = text; // Store the text used for this texture
            std::cout << "Generated QR Code texture for: " << text << " (ID: " << m_qrTextureId << ")" << std::endl;
        } else {
             m_lastGeneratedQrText = ""; // Generation failed
             std::cerr << "QR Code texture generation helper failed for: " << text << std::endl;
         }
    } catch (const std::exception& e) {
        std::cerr << "Error generating QR Code: " << e.what() << std::endl;
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Helper to release the QR Code texture (Now a private member)
void UIQrCodeWindow::releaseQrTexture() {
    if (m_qrTextureId != 0) {
        std::cout << "Deleting QR Code texture (ID: " << m_qrTextureId << ") for text: " << m_lastGeneratedQrText << std::endl;
        glDeleteTextures(1, &m_qrTextureId);
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Draw method implementation
void UIQrCodeWindow::Draw(bool isServerRunning, int serverPort, const std::string& serverIP,
                          std::function<void()> refreshIpCallback)
{
    ImGui::Begin(m_translator.get("qr_code_window_title").c_str());

    bool isIpValid = (serverIP.find("Error") == std::string::npos &&
                      serverIP.find("Fetching") == std::string::npos &&
                      serverIP.find("No suitable") == std::string::npos &&
                      !serverIP.empty());

    if (isServerRunning && serverPort > 0 && isIpValid) {
        std::string http_address = "http://" + serverIP + ":" + std::to_string(serverPort);

        // Regenerate texture only if the address has changed
        if (http_address != m_lastGeneratedQrText) {
            generateQrTexture(http_address);
        }

        ImGui::TextUnformatted(m_translator.get("scan_qr_code_prompt_1").c_str());
        ImGui::TextUnformatted(m_translator.get("scan_qr_code_prompt_2").c_str());

        if (m_qrTextureId != 0) {
            ImGui::Image((ImTextureID)(intptr_t)m_qrTextureId, ImVec2(200, 200));
        } else {
            ImGui::TextUnformatted(m_translator.get("qr_code_failed").c_str());
        }

        ImGui::Text("%s %s", m_translator.get("open_in_browser_label").c_str(), http_address.c_str());
        if (ImGui::Button(m_translator.get("copy_web_address_button").c_str())) {
             ImGui::SetClipboardText(http_address.c_str());
        }

    } else if (!isServerRunning) {
        ImGui::TextUnformatted(m_translator.get("server_stopped_qr_prompt").c_str());
        // Release texture if server stops
        if (m_qrTextureId != 0) { releaseQrTexture(); }
    } else { // Server running but IP invalid/fetching
        ImGui::TextUnformatted(m_translator.get("waiting_for_ip_qr_prompt").c_str());
        if (ImGui::Button(m_translator.get("retry_fetch_ip_button").c_str())) {
            if (refreshIpCallback) {
                refreshIpCallback();
            }
        }
        // Release texture if IP becomes invalid
         if (m_qrTextureId != 0) { releaseQrTexture(); }
    }
    ImGui::End();
}
