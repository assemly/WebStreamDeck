#include "Application.hpp"

// Include necessary headers moved from main.cpp
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem> // <-- Add for std::filesystem
#include <chrono>    // <-- Add for timestamps
#include <ctime>     // <-- Add for timestamps
#include <sstream>   // <-- Add for timestamp formatting
#include <iomanip>   // <-- Add for timestamp formatting
#include <shellapi.h> // <-- Add for Shell_NotifyIconW

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32 // <<< Define before including glfw3.h
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>   // <<< Include for glfwGetWin32Window
#include <GL/glew.h> 

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#endif

// Include manager headers (adjust paths if needed)
#include "Managers/UIManager.hpp" 
#include "Managers/ConfigManager.hpp" 
#include "Managers/ActionRequestManager.hpp"
#include "Managers/NetworkManager.hpp"
#include "Managers/TranslationManager.hpp"
#include "Utils/InputUtils.hpp"
#include "Utils/TextureLoader.hpp"
#include "Utils/IconUtils.hpp" // Include the header for icon conversion

// --- Forward Declarations (if needed, e.g., for callbacks) ---
// static void glfw_error_callback(int error, const char* description);
// static void drop_callback(GLFWwindow* window, int count, const char** paths);

#ifdef _WIN32
// --- Forward Declarations for Static Callbacks (Win32 only) ---
static void window_iconify_callback(GLFWwindow* window, int iconified);
static void window_close_callback(GLFWwindow* window);

// --- Tray Icon Constants and Globals ---
const UINT WM_APP_TRAYMSG = WM_APP + 1; // Custom message for tray icon events
const UINT ID_TRAY_ICON = 1001;         // Identifier for the tray icon
const UINT ID_MENU_SHOW = 2001;         // ID for the 'Show' menu item
const UINT ID_MENU_EXIT = 2002;         // ID for the 'Exit' menu item
const wchar_t* HIDDEN_WND_CLASS_NAME = L"WebStreamDeckMessageWindowClass";
Application* Application::s_instance = nullptr; // Initialize static instance pointer

// --- Helper functions (GetExecutableDirectory, LogToFile) ---
std::wstring GetExecutableDirectory() {
    wchar_t pathBuffer[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);
    if (pathLen > 0 && pathLen < MAX_PATH) {
        try {
            std::filesystem::path exeFSPath = pathBuffer;
            return exeFSPath.parent_path().wstring();
        } catch (...) { // Catch potential filesystem errors
            return L""; // Return empty on error
        }
    }
    return L""; // Return empty on error
}

void LogToFile(const std::wstring& message) {
    static std::wstring logFilePath = L"";
    if (logFilePath.empty()) {
        std::wstring exeDir = GetExecutableDirectory();
        if (!exeDir.empty()) {
            logFilePath = exeDir + L"\\startup_log.txt";
        } else {
            // Fallback if getting exe dir fails (less likely but possible)
            logFilePath = L"startup_log.txt"; // Log to current dir as fallback
        }
         // Log the path being used
         std::wofstream initialLog(logFilePath, std::ios::app);
         if(initialLog.is_open()) {
             initialLog << L"[---- Log Initialized ---- Log File Path: " << logFilePath << L" ----]" << std::endl;
             initialLog.close();
         }
    }

    std::wofstream logFile(logFilePath, std::ios::app); // Open in append mode
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c); // Use localtime_s for safety

        // Format timestamp (YYYY-MM-DD HH:MM:SS)
        std::wstringstream wss;
        wss << std::put_time(&now_tm, L"%Y-%m-%d %H:%M:%S");

        logFile << L"[" << wss.str() << L"] " << message << std::endl;
        logFile.close();
    } else {
        // Output error to cerr if log file cannot be opened
         std::wcerr << L"!!! FATAL: Could not open log file: " << logFilePath << std::endl;
    }
}

// --- Static WndProc for the hidden message window ---
LRESULT CALLBACK Application::TrayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_APP_TRAYMSG && s_instance) {
        switch (lParam) {
            case WM_LBUTTONUP: // Left click - Restore window
                 LogToFile(L"Tray icon left-clicked. Restoring window.");
                 if (s_instance->m_hwnd) {
                     ShowWindow(s_instance->m_hwnd, SW_RESTORE);
                     SetForegroundWindow(s_instance->m_hwnd);
                     // Optional: Remove tray icon when window is shown
                     // Shell_NotifyIconW(NIM_DELETE, &s_instance->m_nid);
                 }
                 break;
            case WM_RBUTTONUP: // Right click - Show menu
                LogToFile(L"Tray icon right-clicked. Showing menu.");
                s_instance->ShowTrayMenu();
                break;
        }
        return 0; // Handled
    }
    
    if (message == WM_COMMAND && s_instance) {
        int wmId = LOWORD(wParam);
        switch (wmId) {
            case ID_MENU_SHOW:
                LogToFile(L"Tray menu 'Show' clicked.");
                 if (s_instance->m_hwnd) {
                     ShowWindow(s_instance->m_hwnd, SW_RESTORE);
                     SetForegroundWindow(s_instance->m_hwnd);
                 }
                break;
            case ID_MENU_EXIT:
                LogToFile(L"Tray menu 'Exit' clicked. Requesting close.");
                if (s_instance->m_window) {
                    // Tell the main loop to exit
                    glfwSetWindowShouldClose(s_instance->m_window, GLFW_TRUE);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0; // Handled
    }

    if (message == WM_DESTROY) {
        // Clean up menu if the message window is somehow destroyed early
        if (s_instance && s_instance->m_trayMenu) {
            DestroyMenu(s_instance->m_trayMenu);
            s_instance->m_trayMenu = NULL;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

// --- Static Callbacks ---
static void glfw_error_callback(int error, const char* description)
{
    // Use std::cerr for consistency
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    // Get the Application instance from the window's user pointer
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) {
        std::cerr << "[GLFW Callback] Error: Could not retrieve Application instance from user pointer." << std::endl;
        return; // Cannot proceed without the app instance
    }

    std::cout << "[GLFW Callback] Drop event detected with " << count << " item(s)." << std::endl;
    #ifdef _WIN32
        UINT acp = GetACP(); 
        std::cout << "[GLFW Callback] System Active Code Page: " << acp << " (Note: Assuming GLFW provides UTF-8)" << std::endl;
        app->m_droppedFilesW.clear(); // <<< Access member variable
        for (int i = 0; i < count; ++i) {
            if (paths[i]) {
                int utf8Len = MultiByteToWideChar(CP_UTF8, 0, paths[i], -1, NULL, 0);
                if (utf8Len > 0) {
                    std::wstring wPath(utf8Len, 0);
                    MultiByteToWideChar(CP_UTF8, 0, paths[i], -1, &wPath[0], utf8Len);
                    wPath.pop_back();
                    app->m_droppedFilesW.push_back(wPath); // <<< Access member variable
                     std::cout << "  - Dropped path added (as wstring). Original: " << paths[i] << std::endl; 
                } else {
                    std::cerr << "[GLFW Callback] Failed to convert dropped path to wstring: " << paths[i] << std::endl;
                }
            }
        }
    #else
        app->m_droppedFiles.clear(); // <<< Access member variable
        for (int i = 0; i < count; ++i) {
            if (paths[i]) { 
                app->m_droppedFiles.push_back(std::string(paths[i])); // <<< Access member variable
                std::cout << "  - Dropped path added: " << paths[i] << std::endl;
            }
        }
    #endif
}

// --- Helper to read startMinimized setting --- 
#ifdef _WIN32
bool Application::ReadStartMinimizedSetting() {
    // Determine config file path relative to executable
    std::wstring exeDir = GetExecutableDirectory();
    if (exeDir.empty()) {
        LogToFile(L"ReadStartMinimizedSetting: Failed to get executable directory.");
        return false; // Cannot read config without path
    }
    std::wstring configFilePath = exeDir + L"\\sysconfig.ini";
    LogToFile(L"ReadStartMinimizedSetting: Reading config from " + configFilePath);

    std::ifstream configFile(configFilePath); 
    if (!configFile.is_open()) {
        LogToFile(L"ReadStartMinimizedSetting: Config file not found or cannot be opened. Assuming false.");
        return false; // Default to false if file doesn't exist
    }

    std::string line;
    bool found = false;
    bool value = false;
    while (std::getline(configFile, line)) {
        // Use the same trim function if available, or implement basic trimming
        // Basic trim example:
         line.erase(0, line.find_first_not_of(" \t\r\n"));
         line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#' || line[0] == ';') { continue; }

        size_t separatorPos = line.find('=');
        if (separatorPos != std::string::npos) {
            std::string key = line.substr(0, separatorPos);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);

            if (key == "startMinimized") {
                std::string valStr = line.substr(separatorPos + 1);
                valStr.erase(0, valStr.find_first_not_of(" \t"));
                valStr.erase(valStr.find_last_not_of(" \t") + 1);
                value = (valStr == "true" || valStr == "1");
                found = true;
                break; // Found the key
            }
        }
    }
    configFile.close();

    if (found) {
        LogToFile(L"ReadStartMinimizedSetting: Found startMinimized = " + std::wstring(value ? L"true" : L"false"));
        return value;
    } else {
        LogToFile(L"ReadStartMinimizedSetting: Key 'startMinimized' not found in config. Assuming false.");
        return false; // Default to false if key not found
    }
}
#endif

Application::Application()
    // Initialize managers in the initializer list - they now just store paths/config
    : m_configManager(),
      m_translationManager("assets/lang", "zh"), // Still construct here, but it does less
      m_networkManager(m_configManager),
      m_actionRequestManager(m_configManager, m_soundService),
      m_uiManager(this->m_configManager, this->m_actionRequestManager, this->m_translationManager, this->m_networkManager),
      m_soundService() // <<< ADDED: Default construct sound service
{
#ifdef _WIN32
    s_instance = this; // Set the static instance pointer
    LogToFile(L"--- Application Constructor Start ---");
    SetupWorkingDirectory();
    // <<< Read the setting AFTER setting the working directory >>>
    m_startMinimized = ReadStartMinimizedSetting(); 
#endif

    // --- Load resources AFTER setting CWD ---
    LogToFile(L"Initializing TranslationManager...");
    if (!m_translationManager.Initialize()) { // <<< Call the new Initialize method
        LogToFile(L"Error: Failed to initialize TranslationManager (loading languages failed).");
        // Decide how to handle this - exit? continue with no translations?
        std::cerr << "[Application] CRITICAL ERROR: Failed to load any language files. Exiting." << std::endl;
        // Possibly set a flag to prevent further initialization or exit gracefully
        // For now, we'll log and continue, but UI might show '???'
        // If you want to exit: throw std::runtime_error("Failed to load translations");
    } else {
        LogToFile(L"TranslationManager Initialized Successfully.");
    }
    
    // TODO: Add similar LoadConfig() call here if ConfigManager is refactored
    // LogToFile(L"Initializing ConfigManager...");
    // if (!m_configManager.LoadConfig()) { ... }

    // --- Continue with other initialization ---
    m_initialized = false; // Initialization not complete yet

    #ifdef _WIN32
    // Set console output code page to UTF-8 on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    LogToFile(L"Initializing Platform...");
    if (!InitializePlatform()) {
        LogToFile(L"Error: Failed to initialize platform.");
        std::cerr << "[Application] Failed to initialize platform." << std::endl;
        // No need to call Shutdown here, destructor will handle it
        return; // Stop initialization
    }

    LogToFile(L"Initializing ImGui...");
    if (!InitializeImGui()) {
         LogToFile(L"Error: Failed to initialize ImGui.");
        std::cerr << "[Application] Failed to initialize ImGui." << std::endl;
        // No need to call Shutdown here, destructor will handle it
        return;
    }

    LogToFile(L"Setting up Callbacks...");
    SetupCallbacks(); // Setup callbacks after managers are ready

    LogToFile(L"Starting Services...");
    if (!StartServices()) { 
        LogToFile(L"Error: Failed to start services.");
        std::cerr << "[Application] Failed to start services." << std::endl;
        // Destructor will handle shutdown
        return;
    }

    LogToFile(L"Application Initialization successful.");
    m_initialized = true; // Mark initialization as complete
}

Application::~Application() {
    std::cout << "Shutting down Application..." << std::endl;
    ShutdownServices(); // Shutdown services first
    // Managers are destroyed automatically when Application goes out of scope
    ShutdownImGui();
    ShutdownPlatform();
#ifdef _WIN32
    // Ensure tray icon is removed (might be redundant if ShutdownPlatform calls it)
    RemoveTrayIcon(); 
    // Destroy the hidden window explicitly if it exists
    if (m_messageHwnd) {
        DestroyWindow(m_messageHwnd);
        m_messageHwnd = NULL;
    }
    UnregisterClassW(HIDDEN_WND_CLASS_NAME, GetModuleHandleW(NULL));
#endif
}

// --- Implementation of SetupWorkingDirectory ---
#ifdef _WIN32
void Application::SetupWorkingDirectory() {
    LogToFile(L"SetupWorkingDirectory: Attempting to set working directory.");
    wchar_t pathBuffer[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);

    if (pathLen > 0 && pathLen < MAX_PATH) {
         std::wstring exePathStr = pathBuffer;
         LogToFile(L"SetupWorkingDirectory: GetModuleFileNameW successful. Path: " + exePathStr);
        try {
            std::filesystem::path exeFSPath = pathBuffer;
            std::filesystem::path exeDir = exeFSPath.parent_path();
             LogToFile(L"SetupWorkingDirectory: Extracted directory: " + exeDir.wstring());

            if (!exeDir.empty()) {
                if (SetCurrentDirectoryW(exeDir.c_str())) {
                    LogToFile(L"SetupWorkingDirectory: SetCurrentDirectoryW successful.");
                } else {
                    DWORD error = GetLastError();
                    LogToFile(L"SetupWorkingDirectory: SetCurrentDirectoryW FAILED. Error code: " + std::to_wstring(error));
                    std::wcerr << L"[Application] Error: Failed to set working directory. Error code: " << error << std::endl;
                }
            } else {
                 LogToFile(L"SetupWorkingDirectory: Extracted directory is empty.");
                 std::wcerr << L"[Application] Error: Could not extract directory from executable path." << std::endl;
            }
        } catch (const std::filesystem::filesystem_error& e) {
             std::string errorMsg = e.what(); // Get error message as std::string
             LogToFile(L"SetupWorkingDirectory: Filesystem exception: " + std::wstring(errorMsg.begin(), errorMsg.end())); // Convert to wstring for logging
             std::cerr << "[Application] Filesystem exception while setting working directory: " << errorMsg << std::endl;
        } catch (const std::exception& e) {
             std::string errorMsg = e.what();
             LogToFile(L"SetupWorkingDirectory: Generic exception: " + std::wstring(errorMsg.begin(), errorMsg.end()));
             std::cerr << "[Application] Generic exception while setting working directory: " << errorMsg << std::endl;
        }
    } else {
        DWORD error = GetLastError();
        LogToFile(L"SetupWorkingDirectory: GetModuleFileNameW FAILED or path too long. Error code: " + std::to_wstring(error));
        std::cerr << "[Application] Error: Could not get executable path to set working directory. Error code: " << error << std::endl;
    }
    LogToFile(L"--- SetupWorkingDirectory End ---");
}
#endif // _WIN32

// --- Initialization Steps ---

bool Application::InitializePlatform() {
    std::cout << "Initializing Platform (GLFW, GLEW)..." << std::endl;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "[Platform] GLFW initialization failed." << std::endl;
        return false;
    }

    // Use GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130"; // Store this? Maybe not needed here
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    m_window = glfwCreateWindow(1280, 720, "WebStreamDeck", NULL, NULL);
    if (m_window == NULL) {
        std::cerr << "[Platform] Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);

    // --- Set Window Icon (Windows only) --- 
    #ifdef _WIN32
    std::cout << "[Platform] Attempting to set window icon..." << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    // Load the icon resource using the ID defined in WebStreamDeck.rc (101)
    HICON hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(101)); 
    if (hIcon) {
        std::cout << "[Platform] Loaded icon resource handle." << std::endl;
        // Convert HICON to RGBA pixels using our utility function
        auto imageDataOpt = IconUtils::ConvertHIconToRGBA(hIcon);
        if (imageDataOpt) {
            std::cout << "[Platform] Converted icon to RGBA." << std::endl;
            GLFWimage glfwImage;
            glfwImage.width = imageDataOpt->width;
            glfwImage.height = imageDataOpt->height;
            glfwImage.pixels = imageDataOpt->pixels.data();

            // Set the icon for the window
            glfwSetWindowIcon(m_window, 1, &glfwImage);
            std::cout << "[Platform] Window icon set successfully." << std::endl;
        } else {
            std::cerr << "[Platform] Failed to convert HICON to RGBA pixels." << std::endl;
        }
        // Destroy the loaded icon handle regardless of conversion success
        DestroyIcon(hIcon);
        std::cout << "[Platform] Destroyed icon resource handle." << std::endl;
    } else {
        std::cerr << "[Platform] Failed to load icon resource (ID: 101). Error: " << GetLastError() << std::endl;
    }
    #endif
    // --- End Set Window Icon ---

    // Set window user pointer
    glfwSetWindowUserPointer(m_window, this);
    std::cout << "[Platform] Set window user pointer." << std::endl;

    // Initialize GLEW *after* creating the OpenGL context
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {   
        // Use iostream instead of fprintf
        std::cerr << "[Platform] Error initializing GLEW: " << glewGetErrorString(err) << std::endl;
        glfwDestroyWindow(m_window); // Clean up window before terminating
        glfwTerminate();
        return false;
    }

    glfwSwapInterval(1); // Enable vsync

    std::cout << "[Platform] Initialization successful." << std::endl;

#ifdef _WIN32
    // --- Get HWND after creating GLFW window ---
    m_hwnd = glfwGetWin32Window(m_window);
    if (!m_hwnd) {
        LogToFile(L"InitializePlatform: Failed to get HWND from GLFW window.");
        std::cerr << "[Platform] Failed to get HWND from GLFW window." << std::endl;
        // Handle error - maybe can't proceed with tray?
    } else {
        LogToFile(L"InitializePlatform: HWND obtained successfully.");
         // --- Initialize Tray Icon --- 
         InitTrayIcon();

         // --- Hide window if starting minimized ---
         if (m_startMinimized) {
             LogToFile(L"InitializePlatform: Hiding main window because startMinimized is true.");
             ShowWindow(m_hwnd, SW_HIDE); // Hide window immediately
         }
    }
#endif

    return true;
}

bool Application::InitializeImGui() {
    std::cout << "Initializing ImGui..." << std::endl;

    // Use GL 3.0 + GLSL 130 (declared locally for now, maybe make member?)
    const char* glsl_version = "#version 130";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        std::cerr << "[ImGui] Failed to initialize ImGui GLFW backend." << std::endl;
        ImGui::DestroyContext(); // Clean up context
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "[ImGui] Failed to initialize ImGui OpenGL3 backend." << std::endl;
        ImGui_ImplGlfw_Shutdown(); // Clean up GLFW backend
        ImGui::DestroyContext();
        return false;
    }

    // Load Fonts
    ImFontAtlas* fonts = io.Fonts;
    fonts->Clear();
    std::string fontPath = "assets/fonts/NotoSansSC-VariableFont_wght.ttf"; 
    float fontSize = 18.0f;
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false;
    fontConfig.PixelSnapH = true;
    // Check if font file exists? (Simple check)
    // Note: std::filesystem might be better but adds dependency if not already used
    FILE* file = fopen(fontPath.c_str(), "rb");
    if (file) {
        fclose(file);
        fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig, fonts->GetGlyphRangesChineseFull());
        std::cout << "[ImGui] Loaded font: " << fontPath << std::endl;
    } else {
        std::cerr << "[ImGui] Warning: Font file not found at " << fontPath << ". Using default font." << std::endl;
        // ImGui will use its default font if none are loaded successfully
    }
    
    // Font building is typically handled automatically by the backend NewFrame call, no explicit build needed here.

    std::cout << "[ImGui] Initialization successful." << std::endl;
    return true;
}

// Managers are now initialized in the constructor initializer list
/* bool Application::InitializeManagers() {
     std::cout << "Initializing Managers..." << std::endl;
     // If using pointers:
     // m_configManager = new ConfigManager();
     // m_translationManager = new TranslationManager("assets/lang", "zh");
     // m_networkManager = new NetworkManager(*m_configManager);
     // m_actionRequestManager = new ActionRequestManager(*m_configManager);
     // m_uiManager = new UIManager(*m_configManager, *m_actionRequestManager, *m_translationManager, *m_networkManager);
     // Add null checks for allocations
     std::cout << "[Managers] Initialization complete (via initializer list)." << std::endl;
    return true; 
} */

void Application::SetupCallbacks() {
    std::cout << "Setting up Callbacks..." << std::endl;
    LogToFile(L"SetupCallbacks called.");

    // Setup GLFW Drop callback
    glfwSetDropCallback(m_window, drop_callback); 
    std::cout << "[Callbacks] GLFW drop callback set." << std::endl;

    // Setup WebSocket message handler
    // Need access to ActionRequestManager instance (m_actionRequestManager)
    m_networkManager.set_websocket_message_handler(
        [this](const json& payload, bool /*isBinary*/) { 
            try {
                // Accessing member variable m_actionRequestManager
                if (payload.contains("type") && payload["type"].is_string() && payload["type"] == "button_press") {
                    if (payload.contains("payload") && payload["payload"].is_object()) {
                        const auto& buttonPayload = payload["payload"];
                        if (buttonPayload.contains("button_id") && buttonPayload["button_id"].is_string()) {
                            std::string buttonId = buttonPayload["button_id"].get<std::string>();
                            std::cout << "[WS Handler] Received button press for ID: " << buttonId << std::endl;
                            this->m_actionRequestManager.requestAction(buttonId); // Use this->
                        } else {
                            std::cerr << "[WS Handler] Missing or invalid 'button_id' in payload." << std::endl;
                        }
                    } else {
                        std::cerr << "[WS Handler] Missing or invalid 'payload' object." << std::endl;
                    }
                } else {
                    std::cerr << "[WS Handler] Received unknown message type or format." << std::endl;
                }
            } catch (const json::exception& e) {
                std::cerr << "[WS Handler] JSON processing error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[WS Handler] General error: " << e.what() << std::endl;
            }
        }
    );
     std::cout << "[Callbacks] WebSocket message handler set." << std::endl;

#ifdef _WIN32
    // Setup Window Iconify (Minimize) Callback
    glfwSetWindowIconifyCallback(m_window, window_iconify_callback);
    LogToFile(L"SetupCallbacks: Set window iconify callback.");
    std::cout << "[Callbacks] GLFW iconify callback set." << std::endl;

    // Setup Window Close Callback
    glfwSetWindowCloseCallback(m_window, window_close_callback);
    LogToFile(L"SetupCallbacks: Set window close callback.");
    std::cout << "[Callbacks] GLFW close callback set." << std::endl;
#endif
}

bool Application::StartServices() {
     std::cout << "Starting Services..." << std::endl;

    // <<< ADDED: Initialize Sound Service >>>
    std::cout << "[Services] Initializing Sound Service..." << std::endl;
    if (!m_soundService.init()) {
        std::cerr << "[Services] FAILED TO INITIALIZE SOUND SERVICE." << std::endl;
        // Treat as non-fatal for now, but log error
    } else {
        std::cout << "[Services] Sound Service Initialized." << std::endl;
        // Load sounds
        std::cout << "[Services] Loading sounds..." << std::endl;
        // --- MODIFIED: Scan directory instead of hardcoding ---
        std::string soundDir = "assets/sounds";
        int loadedCount = 0;
        int attemptedCount = 0;
        try {
            if (std::filesystem::exists(soundDir) && std::filesystem::is_directory(soundDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(soundDir)) {
                    if (entry.is_regular_file()) {
                        std::filesystem::path filePath = entry.path();
                        std::string extension = filePath.extension().string();
                        std::string filename = filePath.filename().stem().string(); // Get filename without extension

                        // Convert extension to lower case for comparison
                        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                        if (extension == ".wav") {
                            attemptedCount++;
                            // Use lowercase filename as the sound name key
                            std::string soundName = filename;
                            std::transform(soundName.begin(), soundName.end(), soundName.begin(), ::tolower); 
                            
                            if (m_soundService.registerSound(soundName, filePath.string())) {
                                loadedCount++;
                            }
                            // registerSound already prints errors if it fails
                        }
                    }
                }
            } else {
                 std::cerr << "[Services] Error: Sound directory not found or is not a directory: " << soundDir << std::endl;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[Services] Error iterating sound directory: " << e.what() << std::endl;
        }
        std::cout << "[Services] Registered " << loadedCount << " out of " << attemptedCount << " found .wav files." << std::endl;
        // --- END MODIFIED ---
    }
    // <<< END: Initialize Sound Service >>>

    // Start Network Manager
    if (!m_networkManager.start(m_serverPort)) { 
        std::cerr << "[Services] FAILED TO START NETWORK SERVICES ON PORT " << m_serverPort << std::endl;
        return false;
    }
    std::cout << "[Services] Network services started on port " << m_serverPort << std::endl;

    // Initialize Core Audio Control (Windows only)
    #ifdef _WIN32
    if (!InputUtils::InitializeAudioControl()) {
        // Log as warning, don't treat as fatal initialization error
        std::cerr << "[Services] Warning: Failed to initialize Core Audio controls." << std::endl;
    }
    #endif
     std::cout << "[Services] Startup complete." << std::endl;
    return true;
}

// --- Shutdown Steps ---

void Application::ShutdownServices() {
     std::cout << "Shutting down Services..." << std::endl;
    // Stop Network Manager
    std::cout << "[Services] Stopping network services..." << std::endl;
    m_networkManager.stop(); 
    std::cout << "[Services] Network services stopped." << std::endl;

    // <<< ADDED: Shutdown Sound Service >>>
    std::cout << "[Services] Shutting down sound service..." << std::endl;
    m_soundService.shutdown();
    std::cout << "[Services] Sound service shut down." << std::endl;
    // <<< END: Shutdown Sound Service >>>

    // Uninitialize Core Audio Control (Windows only)
    #ifdef _WIN32
    InputUtils::UninitializeAudioControl();
    #endif

    // Release static textures
    TextureLoader::ReleaseStaticTextures(); 
     std::cout << "[Services] Shutdown complete." << std::endl;
}

void Application::ShutdownImGui() {
     std::cout << "Shutting down ImGui..." << std::endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    std::cout << "[ImGui] Shutdown complete." << std::endl;
}

void Application::ShutdownPlatform() {
    std::cout << "Shutting down Platform..." << std::endl;
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
     std::cout << "[Platform] Shutdown complete." << std::endl;
}

// --- Tray Icon Methods Implementation ---
#ifdef _WIN32
void Application::InitTrayIcon() {
    LogToFile(L"InitTrayIcon called.");
    if (!m_hwnd) { 
        LogToFile(L"InitTrayIcon: Main window HWND is null.");
        return; 
    }

    // 1. Register the hidden window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = TrayWndProc; // Use static method
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = HIDDEN_WND_CLASS_NAME;
    if (!RegisterClassExW(&wc)) {
        LogToFile(L"InitTrayIcon: Failed to register hidden window class. Error: " + std::to_wstring(GetLastError()));
        return;
    }

    // 2. Create the hidden message-only window
    m_messageHwnd = CreateWindowExW(
        0, HIDDEN_WND_CLASS_NAME, L"WebStreamDeck Message Handler", 0, 
        0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandleW(NULL), NULL);

    if (!m_messageHwnd) {
        LogToFile(L"InitTrayIcon: Failed to create hidden message window. Error: " + std::to_wstring(GetLastError()));
        return;
    }
    LogToFile(L"InitTrayIcon: Hidden message window created successfully.");

    // 3. Prepare the NOTIFYICONDATA structure
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_messageHwnd; // Messages go to the hidden window
    m_nid.uID = ID_TRAY_ICON;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_APP_TRAYMSG; 
    // Load the same icon used for the window (assuming ID 101)
    m_nid.hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101)); 
    if (!m_nid.hIcon) { 
         LogToFile(L"InitTrayIcon: Failed to load icon resource 101 for tray.");
         // Maybe use a default system icon? LoadIcon(NULL, IDI_APPLICATION);
         m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    // Tooltip text
    wcscpy_s(m_nid.szTip, L"WebStreamDeck");

    // 4. Add the icon *only if* starting minimized
    if (m_startMinimized) {
        LogToFile(L"InitTrayIcon: Adding tray icon because startMinimized is true.");
        if (!Shell_NotifyIconW(NIM_ADD, &m_nid)) {
             LogToFile(L"InitTrayIcon: Shell_NotifyIconW(NIM_ADD) failed. Error: " + std::to_wstring(GetLastError()));
        } else {
             LogToFile(L"InitTrayIcon: Shell_NotifyIconW(NIM_ADD) successful.");
        }
    } else {
         LogToFile(L"InitTrayIcon: Not adding tray icon initially (startMinimized is false).");
    }

    // 5. Create the context menu (but don't show it yet)
    m_trayMenu = CreatePopupMenu();
    if (m_trayMenu) {
        AppendMenuW(m_trayMenu, MF_STRING, ID_MENU_SHOW, L"Show");
        AppendMenuW(m_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m_trayMenu, MF_STRING, ID_MENU_EXIT, L"Exit");
    } else {
        LogToFile(L"InitTrayIcon: Failed to create popup menu.");
    }
}

void Application::RemoveTrayIcon() {
    if (m_nid.hWnd && m_nid.uID) { // Check if it was potentially added
        LogToFile(L"RemoveTrayIcon: Attempting to remove tray icon.");
        if (!Shell_NotifyIconW(NIM_DELETE, &m_nid)) {
            // This might fail if the icon wasn't actually added, which is often okay
             // LogToFile(L"RemoveTrayIcon: Shell_NotifyIconW(NIM_DELETE) failed. Error: " + std::to_wstring(GetLastError()));
        } else {
             LogToFile(L"RemoveTrayIcon: Shell_NotifyIconW(NIM_DELETE) successful.");
        }
        // Clean up icon handle loaded by LoadIcon
        if (m_nid.hIcon) {
            DestroyIcon(m_nid.hIcon);
            m_nid.hIcon = NULL;
        }
    }
     // Destroy menu if it exists
     if (m_trayMenu) {
         DestroyMenu(m_trayMenu);
         m_trayMenu = NULL;
     }
}

void Application::ShowTrayMenu() {
    if (!m_trayMenu) {
        LogToFile(L"ShowTrayMenu: Tray menu handle is null.");
        return;
    }
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_messageHwnd); // Required for TrackPopupMenu
    TrackPopupMenu(m_trayMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_messageHwnd, NULL);
    PostMessage(m_messageHwnd, WM_NULL, 0, 0); // Required for TrackPopupMenu
}

// --- GLFW Callbacks Implementation ---
#ifdef _WIN32
static void window_iconify_callback(GLFWwindow* window, int iconified)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app || !app->m_nid.hWnd) return;

    if (iconified) {
        LogToFile(L"window_iconify_callback: Window minimized.");
        ShowWindow(app->m_hwnd, SW_HIDE);
        if (!Shell_NotifyIconW(NIM_ADD, &app->m_nid)) {
            LogToFile(L"window_iconify_callback: Shell_NotifyIconW(NIM_ADD) failed. Error: " + std::to_wstring(GetLastError()));
        }
    } else {
        LogToFile(L"window_iconify_callback: Window restored (iconify=false).");
    }
}

static void window_close_callback(GLFWwindow* window)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
     if (!app || !app->m_nid.hWnd) {
          glfwSetWindowShouldClose(window, GLFW_TRUE);
          return;
     }
    
    LogToFile(L"window_close_callback: Intercepted close request.");
    ShowWindow(app->m_hwnd, SW_HIDE);
    if (!Shell_NotifyIconW(NIM_ADD, &app->m_nid)) {
        LogToFile(L"window_close_callback: Shell_NotifyIconW(NIM_ADD) failed. Error: " + std::to_wstring(GetLastError()));
    }
    glfwSetWindowShouldClose(window, GLFW_FALSE);
}
#endif

// --- Main Loop ---

void Application::Run() {
    if (!m_initialized) { 
       std::cerr << "Application cannot run, initialization failed." << std::endl;
       return;
    }

    std::cout << "Starting main loop..." << std::endl;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
    ImGuiIO& io = ImGui::GetIO(); // Get IO reference

    while (!glfwWindowShouldClose(m_window))
    {   
        // Check close flag status (moved from main)
        // if (glfwWindowShouldClose(m_window)) { ... log ... }
        
        // Poll events (this will trigger drop_callback if files are dropped)
        glfwPollEvents();

        // Process dropped files collected by the callback
        #ifdef _WIN32
        if (!m_droppedFilesW.empty()) {
            std::cout << "[App Run] Processing " << m_droppedFilesW.size() << " dropped files (Windows)." << std::endl;
            // Create a copy to pass, then clear member immediately
            auto filesToProcess = m_droppedFilesW;
            m_droppedFilesW.clear(); 
            // TODO: Call a method on m_uiManager to handle these files
             m_uiManager.ProcessDroppedFiles(filesToProcess);
             std::cout << "[App Run] Cleared dropped files list (Windows)." << std::endl;
        }
        #else
        if (!m_droppedFiles.empty()) {
             std::cout << "[App Run] Processing " << m_droppedFiles.size() << " dropped files." << std::endl;
            auto filesToProcess = m_droppedFiles;
            m_droppedFiles.clear();
            // TODO: Call a method on m_uiManager to handle these files
             m_uiManager.ProcessDroppedFiles(filesToProcess);
            std::cout << "[App Run] Cleared dropped files list." << std::endl;
        }
        #endif

        // Process pending actions
        m_actionRequestManager.processPendingActions();

        // Update server status in UIManager
        m_uiManager.setServerStatus(m_networkManager.is_running(), m_serverPort);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw the UI managed by UIManager
        m_uiManager.drawUI();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows (if enabled)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {   
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(m_window);
    }
    std::cout << "Exited main loop." << std::endl;
} 

#endif // _WIN32 <-- Ensure this matches the #ifdef around Tray Icon Methods Implementation 