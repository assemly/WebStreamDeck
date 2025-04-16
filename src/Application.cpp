#include "Application.hpp"

// Include necessary headers moved from main.cpp
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
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

Application::Application()
    // Initialize managers in the initializer list if they are direct members
    // Requires default constructors or constructors taking known values (like m_serverPort)
    // Order matters here!
    : m_configManager(), // Default constructor
      m_translationManager("assets/lang", "zh"), // Constructor with args
      m_networkManager(m_configManager), // Depends on m_configManager
      m_actionRequestManager(m_configManager), // Depends on m_configManager
      m_uiManager(this->m_configManager, this->m_actionRequestManager, this->m_translationManager, this->m_networkManager) // Depends on several managers
{
    m_initialized = false;
    std::cout << "Initializing Application..." << std::endl;

    #ifdef _WIN32
    // Set console output code page to UTF-8 on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    if (!InitializePlatform()) {
        std::cerr << "[Application] Failed to initialize platform." << std::endl;
        return; // Stop initialization
    }

    if (!InitializeImGui()) {
        std::cerr << "[Application] Failed to initialize ImGui." << std::endl;
        ShutdownPlatform(); // Clean up platform if ImGui fails
        return;
    }

    // Managers are now initialized via initializer list
    // if (!InitializeManagers()) { 
    //     std::cerr << "[Application] Failed to initialize managers." << std::endl;
    //     ShutdownImGui();
    //     ShutdownPlatform();
    //     return;
    // }

    SetupCallbacks(); // Setup callbacks after managers are ready

    if (!StartServices()) { 
        std::cerr << "[Application] Failed to start services." << std::endl;
        // ShutdownManagers? Cleanup is complex if managers hold resources
        ShutdownImGui();
        ShutdownPlatform();
        return;
    }

    std::cout << "[Application] Initialization successful." << std::endl;
    m_initialized = true; // Mark initialization as complete
}

Application::~Application() {
    std::cout << "Shutting down Application..." << std::endl;
    ShutdownServices(); // Shutdown services first
    // Managers are destroyed automatically when Application goes out of scope
    ShutdownImGui();
    ShutdownPlatform();
}

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
}

bool Application::StartServices() {
     std::cout << "Starting Services..." << std::endl;
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