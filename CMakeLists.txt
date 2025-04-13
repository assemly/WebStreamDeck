cmake_minimum_required(VERSION 3.15)
project(WebStreamDeckMinimal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies via vcpkg (requires CMAKE_TOOLCHAIN_FILE to be set)
find_package(imgui CONFIG REQUIRED COMPONENTS core glfw-binding opengl3-binding)
find_package(glfw3 CONFIG REQUIRED) # Usually a dependency of imgui's glfw binding
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Add the executable
add_executable(WebStreamDeckMinimal src/main.cpp)

# Link dependencies
target_link_libraries(WebStreamDeckMinimal PRIVATE
    imgui::imgui # Links core, glfw, and opengl3 bindings
    OpenGL::GL
    Threads::Threads
)

# Optional: Show console window on Windows alongside GUI (uses standard 'main' entry point)
if(WIN32)
    set_target_properties(WebStreamDeckMinimal PROPERTIES WIN32_EXECUTABLE TRUE)
    # target_link_options(WebStreamDeckMinimal PRIVATE "/SUBSYSTEM:WINDOWS") # Use WinMain
    target_link_options(WebStreamDeckMinimal PRIVATE "/SUBSYSTEM:CONSOLE") # Use main
endif() 