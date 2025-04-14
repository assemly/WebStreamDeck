#include "UIButtonGridWindow.hpp"
#include <iostream> // For std::cerr, std::cout
#include <algorithm> // For std::transform

UIButtonGridWindow::UIButtonGridWindow(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager)
    : m_configManager(configManager), m_actionExecutor(actionExecutor), m_translator(translationManager) {}

UIButtonGridWindow::~UIButtonGridWindow() {
    releaseAnimatedGifTextures();
}

void UIButtonGridWindow::releaseAnimatedGifTextures() {
    for (auto const& [path, gifData] : m_animatedGifTextures) {
        if (gifData.loaded) {
            glDeleteTextures(gifData.frameTextureIds.size(), gifData.frameTextureIds.data());
             std::cout << "Deleted " << gifData.frameTextureIds.size() << " GIF textures (GridWindow) for: " << path << std::endl;
        }
    }
    m_animatedGifTextures.clear();
}

void UIButtonGridWindow::DrawSingleButton(const ButtonConfig& button, double currentTime, float buttonSize) {
    ImGui::PushID(button.id.c_str());

    GLuint textureID = 0;
    bool useImageButton = false;
    std::string lowerIconPath = button.icon_path;
    std::transform(lowerIconPath.begin(), lowerIconPath.end(), lowerIconPath.begin(), ::tolower);

    if (!button.icon_path.empty()) {
        if (lowerIconPath.length() > 4 && lowerIconPath.substr(lowerIconPath.length() - 4) == ".gif") {
            // Handle Animated GIF
            auto it = m_animatedGifTextures.find(button.icon_path);
            if (it == m_animatedGifTextures.end()) {
                GifLoader::AnimatedGif gifData;
                if (GifLoader::LoadAnimatedGifFromFile(button.icon_path.c_str(), gifData)) {
                    gifData.lastFrameTime = currentTime;
                    m_animatedGifTextures[button.icon_path] = std::move(gifData);
                    it = m_animatedGifTextures.find(button.icon_path);
                } else {
                     m_animatedGifTextures[button.icon_path] = {}; // Cache failure
                    std::cerr << "Failed to load GIF (GridWindow) for button '" << button.id << "' from path: " << button.icon_path << std::endl;
                }
            }

            if (it != m_animatedGifTextures.end() && it->second.loaded && !it->second.frameTextureIds.empty()) {
                 GifLoader::AnimatedGif& gif = it->second;
                double timeSinceLastFrame = currentTime - gif.lastFrameTime;
                double frameDelaySeconds = static_cast<double>(gif.frameDelaysMs[gif.currentFrame]) / 1000.0;

                if (timeSinceLastFrame >= frameDelaySeconds) {
                    gif.currentFrame = (gif.currentFrame + 1) % gif.frameTextureIds.size();
                    gif.lastFrameTime = currentTime;
                }
                textureID = gif.frameTextureIds[gif.currentFrame];
                useImageButton = true;
            }
        } else {
            // Handle Static Image using the global TextureLoader
            textureID = TextureLoader::LoadTexture(button.icon_path);
            if (textureID != 0) {
                useImageButton = true;
            } else {
                // TextureLoader::LoadTexture already logs errors
            }
        }
    }

    // Button Rendering
    bool buttonClicked = false;
    ImVec2 sizeVec(buttonSize, buttonSize);
    if (useImageButton && textureID != 0) {
         buttonClicked = ImGui::ImageButton(button.id.c_str(), // Use button.id as str_id for image button too
                                            (ImTextureID)(intptr_t)textureID,
                                            sizeVec,
                                            ImVec2(0, 0), // uv0
                                            ImVec2(1, 1), // uv1
                                            ImVec4(0,0,0,0), // bg_col
                                            ImVec4(1,1,1,1)); // tint_col
         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", button.name.c_str());
         }
    } else {
        buttonClicked = ImGui::Button(button.name.c_str(), sizeVec);
    }

    // Handle Click
    if (buttonClicked) {
         printf("Button '%s' (ID: %s) clicked! Action: %s(%s)\n",
               button.name.c_str(), button.id.c_str(),
               button.action_type.c_str(), button.action_param.c_str());
        m_actionExecutor.requestAction(button.id);
    }

    ImGui::PopID();
}

void UIButtonGridWindow::Draw() {
    ImGui::Begin(m_translator.get("button_grid_window_title").c_str());

    const std::vector<ButtonConfig>& buttons = m_configManager.getButtons();
    double currentTime = ImGui::GetTime();

    if (buttons.empty()) {
        ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
    } else {
        const float button_size = 100.0f; // Base button size
        // --- Potential Dynamic Layout --- 
        // You could calculate buttons_per_row based on window width here:
        // float window_width = ImGui::GetContentRegionAvail().x;
        // float spacing = ImGui::GetStyle().ItemSpacing.x;
        // int calculated_cols = static_cast<int>(window_width / (button_size + spacing));
        // const int buttons_per_row = std::max(1, calculated_cols); // Ensure at least 1 column
        const int buttons_per_row = 4; // Keep fixed for now
        // --- End Potential Dynamic Layout ---
        
        int button_index_in_row = 0;

        for (const auto& button : buttons) {
            // Call the helper function to draw the button
            DrawSingleButton(button, currentTime, button_size);

            // Handle layout (wrapping)
            button_index_in_row++;
            if (button_index_in_row < buttons_per_row) {
                ImGui::SameLine();
            } else {
                button_index_in_row = 0; // Reset for the next row
            }
        }

        // Ensure the next item starts on a new line if the last row wasn't full
        // (This logic might need adjustment based on the final layout implementation)
        if (button_index_in_row != 0) {
             ImGui::NewLine();
        }
    }

    ImGui::End();
}