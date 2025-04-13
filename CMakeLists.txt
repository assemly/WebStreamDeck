cmake_minimum_required(VERSION 3.15)
project(WebStreamDeck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add ImGui manually from docking branch
add_library(imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    third_party/imgui
    third_party/imgui/backends
    # ${GLFW_INCLUDE_DIR} # Prefer linking the target instead
)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
# Link glfw3 to the imgui library target so imgui_impl_glfw can find the headers
# and any other necessary flags/libs. Using PUBLIC ensures downstream targets (like the main exe)
# also get linked to glfw3 if needed.
# Try using the target name suggested by vcpkg's output: glfw
target_link_libraries(imgui PUBLIC glfw)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED) # Find nlohmann_json via vcpkg

# Add the executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/UIManager.cpp
    src/ConfigManager.cpp
    src/ActionExecutor.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui       # Links our imgui library (which now pulls in glfw publicly)
    # glfw      # No longer needed here explicitly
    OpenGL::GL
    Threads::Threads
    nlohmann_json::nlohmann_json # Link nlohmann_json target
)


# Optional: Show console window on Windows alongside GUI (uses standard 'main' entry point)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    # target_link_options(WebStreamDeckMinimal PRIVATE "/SUBSYSTEM:WINDOWS") # Use WinMain
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE") # Use main
endif() 