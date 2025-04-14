#pragma once

#include <imgui.h>
#include <string>
#include <map>
#include "ConfigManager.hpp"
#include "ActionExecutor.hpp"
#include "TranslationManager.hpp"
#include "InputUtils.hpp"
#include "GifLoader.hpp"
// Removed: #include <GL/gl.h> // GLuint is now defined via GLEW included in main.cpp or implicitly

// Define GLuint manually if needed, or rely on it being included via other headers
// For clarity, explicitly including <GL/glew.h> might be better if direct GL calls were made here,
// but for just the type GLuint, it's often implicitly available.
// Let's rely on implicit for now. If compilation fails, we might need <glad/gl.h> or ensure glew.h propagation.
// #ifndef GLuint // Basic guard in case it's not defined elsewhere yet
//     typedef unsigned int GLuint;
// #endif

class UIManager
{
public:
    explicit UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager);
    ~UIManager();

    void drawUI();

    // Method to receive server status from main application
    void setServerStatus(bool isRunning, int port);

private:
    ConfigManager& m_configManager;
    ActionExecutor& m_actionExecutor;
    TranslationManager& m_translator;

    // Server status variables
    bool m_isServerRunning = false;
    int m_serverPort = 0;
    std::string m_serverIP = "Fetching..."; // Default value

    GLuint m_qrTextureId = 0; // Texture ID for the QR code
    std::string m_lastGeneratedQrText = ""; // Store the text used for the last generated QR code
    std::map<std::string, GLuint> m_buttonIconTextures; // Added for icon textures
    std::map<std::string, GifLoader::AnimatedGif> m_animatedGifTextures; 

    // State for adding/editing buttons (fields reused for both modes)
    char m_newButtonId[128] = "";
    char m_newButtonName[128] = "";
    int m_newButtonActionTypeIndex = -1; // Index for combo box
    char m_newButtonActionParam[256] = "";
    char m_newButtonIconPath[256] = ""; // Added for icon path
    std::string m_editingButtonId = ""; // ID of the button being edited (if any)
    bool m_isCapturingHotkey = false; // Flag to indicate hotkey capture mode
    bool m_manualHotkeyEntry = false; // ADDED: Flag to toggle manual hotkey input

    // State for delete confirmation
    bool m_showDeleteConfirmation = false;
    std::string m_buttonIdToDelete = "";

    // List of supported action types (could be moved to ConfigManager or enum later)
    const std::vector<std::string> m_supportedActionTypes = {
        "launch_app", 
        "open_url", 
        "hotkey",
        // ADDED: Media control action types
        "media_volume_up",
        "media_volume_down",
        "media_mute",
        "media_play_pause",
        "media_next_track",
        "media_prev_track",
        "media_stop"
    };

    // Private helper methods for drawing specific windows
    void drawButtonGridWindow();
    void drawConfigurationWindow();
    void drawStatusLogWindow();
    void drawQrCodeWindow();

    // Helper to get local IP (implementation in cpp)
    void updateLocalIP();

    void generateQrTexture(const std::string& text); // Helper to generate/update texture
    void releaseQrTexture(); // Helper to release texture

    GLuint LoadTextureFromFile(const char* filename); // Added for icon loading
    void releaseAnimatedGifTextures(); // Keep this for UIManager resource cleanup
}; 