#pragma once

#include <imgui.h>
#include <string>
#include "ConfigManager.hpp"
#include "ActionExecutor.hpp"
// Removed: #include <GL/gl.h> // GLuint is now defined via GLEW included in main.cpp or implicitly

// Define GLuint manually if needed, or rely on it being included via other headers
// For clarity, explicitly including <GL/glew.h> might be better if direct GL calls were made here,
// but for just the type GLuint, it's often implicitly available.
// Let's rely on implicit for now. If compilation fails, we might need <glad/gl.h> or ensure glew.h propagation.
#ifndef GLuint // Basic guard in case it's not defined elsewhere yet
    typedef unsigned int GLuint;
#endif

class UIManager
{
public:
    explicit UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor);
    ~UIManager();

    void drawUI();

    // Method to receive server status from main application
    void setServerStatus(bool isRunning, int port);

private:
    ConfigManager& m_configManager;
    ActionExecutor& m_actionExecutor;

    // Server status variables
    bool m_isServerRunning = false;
    int m_serverPort = 0;
    std::string m_serverIP = "Fetching..."; // Default value

    GLuint m_qrTextureId = 0; // Texture ID for the QR code
    std::string m_lastGeneratedQrText = ""; // Store the text used for the last generated QR code

    // Private helper methods for drawing specific windows
    void drawButtonGridWindow();
    void drawConfigurationWindow();
    void drawStatusLogWindow();
    void drawQrCodeWindow();

    // Helper to get local IP (implementation in cpp)
    void updateLocalIP();

    void generateQrTexture(const std::string& text); // Helper to generate/update texture
    void releaseQrTexture(); // Helper to release texture

    // Add any UI state variables here if needed later
    // bool show_demo_window = false; 
}; 