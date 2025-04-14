#pragma once

#include <imgui.h>
#include <string>
#include <functional> // For std::function
#include <vector>     // For texture data
#include <GL/glew.h>    // For GLuint, gl functions
#include "../TranslationManager.hpp"
#include <qrcodegen.hpp> // For QR code generation

class UIQrCodeWindow {
public:
    UIQrCodeWindow(TranslationManager& translationManager);
    ~UIQrCodeWindow();

    // Draw method takes server status, IP, port, and callbacks
    void Draw(bool isServerRunning, int serverPort, const std::string& serverIP,
              std::function<void()> refreshIpCallback);

private:
    TranslationManager& m_translator;

    // QR Code state moved from UIManager
    GLuint m_qrTextureId = 0;
    std::string m_lastGeneratedQrText = "";

    // Helper functions moved from UIManager (now private members)
    void generateQrTexture(const std::string& text);
    void releaseQrTexture();
    GLuint qrCodeToTextureHelper(const qrcodegen::QrCode& qr); // Made private member
};