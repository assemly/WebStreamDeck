#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

// Define this before including glfw3.h to prevent it from including gl.h
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <GL/glew.h>      // Include GLEW header (now safe after GLFW_INCLUDE_NONE)
#include <memory> // For std::unique_ptr
#include <iostream> // For std::cerr

#include "UIManager.hpp" // Include the new UI Manager header
#include "ConfigManager.hpp" // Include ConfigManager header
#include "ActionExecutor.hpp" // Include ActionExecutor header
#include "CommServer.hpp" // Include CommServer header
#include "TranslationManager.hpp" // Include TranslationManager header
#include "InputUtils.hpp" // For audio control init/uninit

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // Use GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "WebStreamDeck", NULL, NULL); // Adjusted default window size
    if (window == NULL) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW *after* creating the OpenGL context
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return 1;
    }

    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    // Minimal setup - no docking or viewports for simplicity // Remove if docking/viewports are now intended

    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font.
    // - Read https://github.com/ocornut/imgui/blob/master/docs/FONTS.md for more details.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application.
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build().
    // For Chinese characters, you need a font supporting them and to specify the correct glyph ranges.
    ImFontAtlas* fonts = io.Fonts;
    fonts->Clear(); // Clear existing fonts (including default)
    
    // Load font from the assets/fonts directory (relative to executable)
    // MAKE SURE the font file exists in assets/fonts/ and CMakeLists copies it!
    std::string fontPath = "assets/fonts/NotoSansSC-VariableFont_wght.ttf"; // MODIFIED: Use Noto Sans SC Variable font
    float fontSize = 18.0f; // Adjust font size as needed
    
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false; // Replace default font
    fontConfig.PixelSnapH = true;
    // Load default Latin characters + FULL simplified Chinese characters
    fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig, fonts->GetGlyphRangesChineseFull());
    
    // Optional: You could merge icons or other fonts here if needed later
    // Example: fonts->AddFontFromFileTTF("path/to/icons.ttf", fontSize, &fontConfig, icon_ranges);
    
    // Build the font atlas
    // This must be done after adding all fonts
    unsigned char* pixels;
    int width, height;
    fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // No need to explicitly build if using default ImGui_ImplOpenGL3_NewFrame etc.
    // ImGui_ImplOpenGL3_CreateFontsTexture(); // Might be needed depending on backend setup, but often automatic


    // Create Core Managers
    TranslationManager translationManager("assets/lang", "zh"); // Loads default lang (zh)
    ConfigManager configManager;
    ActionExecutor actionExecutor(configManager);
    UIManager uiManager(configManager, actionExecutor, translationManager);

    // Create Communication Server, passing ConfigManager
    auto commServer = std::make_unique<CommServer>(configManager);
    const int webSocketPort = 9002;

    // Define the message handler lambda
    commServer->set_message_handler(
        [&actionExecutor](uWS::WebSocket<false, true, PerSocketData>* /*ws*/, const json& payload, bool /*isBinary*/) {
            try {
                // Check protocol: { "type": "button_press", "payload": { "button_id": "..." } }
                if (payload.contains("type") && payload["type"].is_string() && payload["type"] == "button_press") {
                    if (payload.contains("payload") && payload["payload"].is_object()) {
                        const auto& buttonPayload = payload["payload"];
                        if (buttonPayload.contains("button_id") && buttonPayload["button_id"].is_string()) {
                            std::string buttonId = buttonPayload["button_id"].get<std::string>();
                            std::cout << "Received button press for ID: " << buttonId << std::endl;
                            // UPDATED: Call requestAction instead of executeAction
                            actionExecutor.requestAction(buttonId);
                        } else {
                            std::cerr << "Message handler: Missing or invalid 'button_id' in payload." << std::endl;
                        }
                    } else {
                        std::cerr << "Message handler: Missing or invalid 'payload' object." << std::endl;
                    }
                } else {
                    // Optional: Handle other message types or ignore
                    std::cerr << "Message handler: Received unknown message type or format." << std::endl;
                }
            } catch (const json::exception& e) {
                std::cerr << "Message handler: JSON processing error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Message handler: General error: " << e.what() << std::endl;
            }
        }
    );

    // Start the server
    if (!commServer->start(webSocketPort)) {
        std::cerr << "!!!!!!!! FAILED TO START WEBSOCKET SERVER ON PORT " << webSocketPort << " !!!!!!!!" << std::endl;
        // Decide how to handle failure - maybe exit, maybe continue without server?
        // For now, just print error and continue.
    }

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // Initialize Core Audio Control (Windows only)
#ifdef _WIN32
    if (!InputUtils::InitializeAudioControl()) {
        std::cerr << "Warning: Failed to initialize Core Audio controls." << std::endl;
        // Continue execution even if audio control fails, 
        // volume buttons will just log errors when pressed.
    }
#endif

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // ADDED: Process actions requested from other threads
        actionExecutor.processPendingActions();

        // Update server status in UIManager
        uiManager.setServerStatus(commServer->is_running(), webSocketPort);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Pass server status to UI Manager if needed (e.g., for display or QR code generation)
        uiManager.drawUI();

        // --- All UI drawing logic moved to UIManager --- 

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    std::cout << "Stopping WebSocket server..." << std::endl;
    commServer->stop(); // Stop the server thread before cleaning up ImGui/GLFW
    std::cout << "WebSocket server stopped." << std::endl;

    // Uninitialize Core Audio Control (Windows only)
#ifdef _WIN32
    InputUtils::UninitializeAudioControl();
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
} 