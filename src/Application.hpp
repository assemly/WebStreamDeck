#pragma once

// Include full definitions for manager members instead of forward declarations
#include "Managers/ConfigManager.hpp"
#include "Managers/TranslationManager.hpp"
#include "Managers/NetworkManager.hpp"
#include "Managers/ActionRequestManager.hpp"
#include "Managers/UIManager.hpp"

// Forward declaration only needed for GLFWwindow now
struct GLFWwindow;

class Application {
public:
    Application();
    ~Application();

    // Disallow copy and assign
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Main application loop
    void Run();

    // Dropped file paths (moved from global)
#ifdef _WIN32
    std::vector<std::wstring> m_droppedFilesW;
#else
    std::vector<std::string> m_droppedFiles;
#endif

private:
    // Initialization steps
    bool InitializePlatform(); // GLFW, GLEW
    bool InitializeImGui();    // ImGui context, style, backends, fonts
    bool StartServices();      // NetworkManager start, Audio init

    // Shutdown steps
    void ShutdownServices();   // NetworkManager stop, Audio uninit, Texture release
    void ShutdownImGui();
    void ShutdownPlatform();

    // Helper for setting up callbacks
    void SetupCallbacks();

    // --- Member Variables ---
    GLFWwindow* m_window = nullptr;

    // Managers are now direct members. Order matters for initialization!
    ConfigManager m_configManager; 
    TranslationManager m_translationManager;
    NetworkManager m_networkManager;
    ActionRequestManager m_actionRequestManager;
    UIManager m_uiManager;

    // Keep track of initialization status
    bool m_initialized = false;
    
    // Configurable values (can be moved to ConfigManager later)
    const int m_serverPort = 9002; 
}; 