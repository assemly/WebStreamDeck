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

    // Create the Config Manager instance FIRST
    ConfigManager configManager; // Loads config in constructor

    // Create the Action Executor instance, passing the config manager
    ActionExecutor actionExecutor(configManager);

    // Create the UI Manager instance, passing both managers
    UIManager uiManager(configManager, actionExecutor);

    // Create and Start Communication Server
    auto commServer = std::make_unique<CommServer>();
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
                            // Execute the action
                            actionExecutor.executeAction(buttonId);
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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Pass server status to UI Manager if needed (e.g., for display or QR code generation)
        uiManager.setServerStatus(commServer->is_running(), webSocketPort);
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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
} 