cmake_minimum_required(VERSION 3.15)
project(WebStreamDeck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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
    third_party/imgui/ImGuiFileDialog.cpp
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
find_package(unofficial-uwebsockets CONFIG REQUIRED)  # CORRECTED: Use package name from vcpkg output
find_package(GLEW REQUIRED) # Find GLEW
find_package(Stb REQUIRED) # Find stb via vcpkg (Module mode, Capitalized)
find_package(GIF REQUIRED) 

# Add local third-party library: qrcodegen
add_library(qrcodegen_lib third_party/QR-Code-generator-1.8.0/qrcodegen.cpp)
target_include_directories(qrcodegen_lib PUBLIC third_party/QR-Code-generator-1.8.0)

# Ensure stb directory is included (though stb_image is header-only for includes)
# target_include_directories(${PROJECT_NAME} PRIVATE third_party/stb) # REMOVED this line

# Add the executable
# Store source files in a variable
set(APP_SOURCES
    src/Utils/InputUtils.cpp
    src/Utils/GifLoader.cpp
    src/Utils/NetworkUtils.cpp
    src/Utils/TextureLoader.cpp
    src/Utils/StringUtils.cpp
    src/Utils/IconUtils.cpp

    src/Services/ActionExecutionService.cpp
    src/Services/HttpServer.cpp
    src/Services/WebSocketServer.cpp
    src/Managers/UIManager.cpp
    src/Managers/ConfigManager.cpp
    src/Managers/ActionRequestManager.cpp
    src/Managers/NetworkManager.cpp
    src/Managers/TranslationManager.cpp

    src/UIWindows/Components/PresetManagerComponent.cpp
    src/UIWindows/Components/ButtonListComponent.cpp
    src/UIWindows/Components/ButtonEditComponent.cpp
    src/UIWindows/Components/GridPaginationComponent.cpp
    src/UIWindows/Components/ButtonSelectorPopupComponent.cpp
    src/UIWindows/Components/GridCellComponent.cpp

    src/UIWindows/UIButtonGridWindow.cpp
    src/UIWindows/UIConfigurationWindow.cpp
    src/UIWindows/UIStatusLogWindow.cpp
    src/UIWindows/UIQrCodeWindow.cpp
    src/UIWindows/UILayoutSettingsWindow.cpp

    src/Application.cpp

    src/main.cpp
)

# Add the resource file only on Windows
if(WIN32)
    list(APPEND APP_SOURCES WebStreamDeck.rc)
endif()

add_executable(${PROJECT_NAME} ${APP_SOURCES})

# ADDED: Define NOMINMAX globally to prevent windows.h min/max macro conflicts
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

# Tell ImGui to use GLEW
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui       # Links our imgui library (which now pulls in glfw publicly)
    # glfw      # No longer needed here explicitly
    GLEW::glew # Link GLEW instead
    Threads::Threads
    nlohmann_json::nlohmann_json # Link nlohmann_json target
    unofficial::uwebsockets::uwebsockets     # CORRECTED: Use target name from vcpkg output
    qrcodegen_lib         # Link the local qrcodegen library target
    # stb::stb              # Link stb via vcpkg # REMOVED: stb is likely header-only, no target to link
    GIF::GIF              # ADDED: Link against giflib target (as suggested by vcpkg)
)

# Copy the web directory to the executable output directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web # Source directory (project root/web)
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/web # Destination directory (executable dir/web)
    COMMENT "Copying web assets to output directory"
)


# Copy the fonts directory from assets to the executable output directory after build
# NOTE: Adjust the filename if you are using a different font
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different # Copy the entire directory
        ${CMAKE_SOURCE_DIR}/assets# Source fonts directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets # Destination fonts directory
    COMMENT "Copying assets to output directory"
)

# 为 MSVC 编译器设置源文件和执行字符集为 UTF-8
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "/utf-8"          # 告诉编译器源文件和执行字符集都为 UTF-8
    )
endif()

# Optional: Show console window on Windows alongside GUI (uses standard 'main' entry point)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    # target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS") # Use WinMain
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE") # Use main
endif() 