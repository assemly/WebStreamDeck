#pragma once

// Include full definitions for manager members instead of forward declarations
#include "Managers/ConfigManager.hpp"
#include "Managers/TranslationManager.hpp"
#include "Managers/NetworkManager.hpp"
#include "Managers/ActionRequestManager.hpp"
#include "Managers/UIManager.hpp"
#include "Services/SoundPlaybackService.hpp"

// Forward declaration only needed for GLFWwindow now
struct GLFWwindow;

#ifdef _WIN32
#include <windows.h> // Include HWND, HMENU etc.
#include <shellapi.h> // Include NOTIFYICONDATAW
#endif

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

#ifdef _WIN32
    // Friend declarations for static GLFW callbacks needing private access
    friend void window_iconify_callback(GLFWwindow* window, int iconified);
    friend void window_close_callback(GLFWwindow* window);
#endif

private:
    // Initialization steps
    void SetupWorkingDirectory();
    bool ReadStartMinimizedSetting();
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
    SoundPlaybackService m_soundService;

    // Keep track of initialization status
    bool m_initialized = false;
    bool m_startMinimized = false;
    
    // Configurable values (can be moved to ConfigManager later)
    const int m_serverPort = 9002; 

#ifdef _WIN32
    // Win32 specific handles and data for Tray Icon
    HWND m_hwnd = NULL; // Handle to the main GLFW window
    HWND m_messageHwnd = NULL; // Handle to the hidden message-only window
    NOTIFYICONDATAW m_nid = {}; // Tray icon data structure
    HMENU m_trayMenu = NULL; // Handle to the right-click context menu
    static Application* s_instance; // Static pointer to the Application instance for WndProc

    // Tray Icon Helpers
    void InitTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    static LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
}; 