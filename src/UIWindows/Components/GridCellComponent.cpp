#include "GridCellComponent.hpp"
#include "../../Utils/TextureLoader.hpp" // For static textures
#include <imgui_internal.h> // Access internal API if needed (e.g., for advanced DND)
#include <iostream>         // For logging
#include <algorithm>        // For std::transform

GridCellComponent::GridCellComponent(ConfigManager& configManager,
                                     TranslationManager& translator,
                                     std::map<std::string, GifLoader::AnimatedGif>& animatedGifTextures)
    : m_configManager(configManager),
      m_translator(translator),
      m_animatedGifTextures(animatedGifTextures) // Store the reference
{}

InteractionResult GridCellComponent::DrawButtonCell(const ButtonConfig& button, int page, int row, int col, double currentTime, const ImVec2& buttonSizeVec) {
    InteractionResult finalResult{ CellInteractionType::NONE };
    finalResult.buttonId = button.id; // Pre-fill buttonId for convenience
    finalResult.page = page;
    finalResult.row = row;
    finalResult.col = col;

    ImGui::PushID(button.id.c_str());

    // Use InvisibleButton for the interaction area
    bool buttonClicked = ImGui::InvisibleButton(button.id.c_str(), buttonSizeVec);
    bool isHovered = ImGui::IsItemHovered();
    bool isActive = ImGui::IsItemActive();

    // Draw the button visuals using a helper
    DrawSingleButtonVisuals(button, currentTime, buttonSizeVec, isHovered, isActive);

    // Handle Drag Source
    HandleButtonDragSource(button, buttonSizeVec);

    // Handle Drop Target and Context Menu, check their results
    InteractionResult dropResult = HandleButtonDropTarget(button.id);
    InteractionResult contextResult = HandleButtonContextMenu(button.id, page, row, col);

    // Prioritize results: DND > ContextMenu > Click
    if (dropResult.type != CellInteractionType::NONE) {
        finalResult = dropResult;
        // Update buttonId if needed (though HandleButtonDropTarget doesn't set it)
        finalResult.buttonId = button.id;
        finalResult.page = page; // Ensure page/row/col are set from button context
        finalResult.row = row;
        finalResult.col = col;
    } else if (contextResult.type != CellInteractionType::NONE) {
        finalResult = contextResult; // Already has buttonId, page, row, col set
    } else if (buttonClicked) {
        finalResult.type = CellInteractionType::BUTTON_CLICKED;
        // ButtonId, page, row, col already set
    }

    ImGui::PopID();
    return finalResult;
}

InteractionResult GridCellComponent::DrawEmptyCell(int page, int row, int col, const ImVec2& buttonSizeVec) {
    InteractionResult finalResult{ CellInteractionType::NONE };
    finalResult.page = page; // Pre-fill location
    finalResult.row = row;
    finalResult.col = col;

    std::string cellPayloadId = "empty_" + std::to_string(page) + "_" + std::to_string(row) + "_" + std::to_string(col);
    ImGui::PushID(cellPayloadId.c_str());

    // Draw simple placeholder or just use InvisibleButton
    ImGui::InvisibleButton("empty_cell_btn", buttonSizeVec); // Unique label
    // Optional: Draw a subtle background for empty cells
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    drawList->AddRectFilled(p_min, p_max, ImColor(40, 40, 40, 150), 4.0f); // Darker, semi-transparent

    // Handle Drop Target and Context Menu
    InteractionResult dropResult = HandleEmptyCellDropTarget(page, row, col);
    InteractionResult contextResult = HandleEmptyCellContextMenu(page, row, col);

    // Prioritize DND over Context Menu for empty cells
    if (dropResult.type != CellInteractionType::NONE) {
        finalResult = dropResult; // Already contains page, row, col
    } else if (contextResult.type != CellInteractionType::NONE) {
        finalResult = contextResult; // Already contains page, row, col
    }

    ImGui::PopID();
    return finalResult;
}

// --- Private Helper Methods ---

void GridCellComponent::DrawSingleButtonVisuals(const ButtonConfig& button, double currentTime, const ImVec2& buttonSizeVec, bool isHovered, bool isActive) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 topLeft = ImGui::GetItemRectMin(); // Use rect from InvisibleButton
    ImVec2 bottomRight = ImGui::GetItemRectMax();

    // Draw background rectangle
    ImU32 buttonBgColor = ImColor(45, 45, 45, 255);
    if (isActive) {
        buttonBgColor = ImColor(75, 75, 75, 255);
    } else if (isHovered) {
        buttonBgColor = ImColor(60, 60, 60, 255);
    }
    drawList->AddRectFilled(topLeft, bottomRight, buttonBgColor, 4.0f);

    // Define padding and calculate content area
    float padding = 8.0f;
    float contentSize = buttonSizeVec.x - 2 * padding;
    ImVec2 contentTopLeft = ImVec2(topLeft.x + padding, topLeft.y + padding);
    if (contentSize < 0) contentSize = 0;
    ImVec2 contentBottomRight = ImVec2(contentTopLeft.x + contentSize, contentTopLeft.y + contentSize);

    GLuint textureID = 0;
    bool useImage = false;
    std::string lowerIconPath = button.icon_path;
    std::transform(lowerIconPath.begin(), lowerIconPath.end(), lowerIconPath.begin(), ::tolower);

    if (!button.icon_path.empty()) {
        if (lowerIconPath.length() > 4 && lowerIconPath.substr(lowerIconPath.length() - 4) == ".gif") {
            // Handle Animated GIF using the referenced map
            auto it = m_animatedGifTextures.find(button.icon_path);
            if (it == m_animatedGifTextures.end()) {
                GifLoader::AnimatedGif gifData;
                if (GifLoader::LoadAnimatedGifFromFile(button.icon_path.c_str(), gifData)) {
                    gifData.lastFrameTime = currentTime;
                    // Use insert or emplace for potentially better performance/safety
                    auto insertResult = m_animatedGifTextures.emplace(button.icon_path, std::move(gifData));
                    it = insertResult.first;
                } else {
                    m_animatedGifTextures[button.icon_path] = {}; // Cache failure
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
                useImage = true;
            }
        } else {
            // Handle Static Image
            textureID = TextureLoader::LoadTexture(button.icon_path);
            if (textureID != 0) {
                useImage = true;
            }
        }
    }

    // Draw image or text
    if (useImage && textureID != 0) {
        drawList->AddImageRounded((ImTextureID)(intptr_t)textureID,
                                contentTopLeft, contentBottomRight,
                                ImVec2(0, 0), ImVec2(1, 1),
                                IM_COL32_WHITE, 0.0f);
    } else {
        ImVec2 textSize = ImGui::CalcTextSize(button.name.c_str());
        ImVec2 textPos = ImVec2(
            contentTopLeft.x + std::max(0.0f, (contentSize - textSize.x) * 0.5f),
            contentTopLeft.y + std::max(0.0f, (contentSize - textSize.y) * 0.5f)
        );
        // Clip text to content area (optional, AddText doesn't clip automatically)
        // ImGui::PushClipRect(contentTopLeft, contentBottomRight, true);
        drawList->AddText(textPos, ImColor(255, 255, 255, 255), button.name.c_str());
        // ImGui::PopClipRect();
    }

    // Tooltip
    if (isHovered) {
        ImGui::SetTooltip("%s", button.name.c_str());
    }
}

void GridCellComponent::HandleButtonDragSource(const ButtonConfig& button, const ImVec2& buttonSizeVec) {
     if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        const char* idPayload = button.id.c_str();
        ImGui::SetDragDropPayload("BUTTON_GRID_ITEM", idPayload, strlen(idPayload) + 1);

        // Draw preview
        GLuint previewTextureID = 0;
        // ... (code to get previewTextureID from GIF map or TextureLoader, similar to DrawSingleButtonVisuals) ...
        std::string lowerIconPath = button.icon_path;
        std::transform(lowerIconPath.begin(), lowerIconPath.end(), lowerIconPath.begin(), ::tolower);
         if (!button.icon_path.empty()) {
             if (lowerIconPath.length() > 4 && lowerIconPath.substr(lowerIconPath.length() - 4) == ".gif") {
                auto it = m_animatedGifTextures.find(button.icon_path);
                if (it != m_animatedGifTextures.end() && it->second.loaded && !it->second.frameTextureIds.empty()) {
                    previewTextureID = it->second.frameTextureIds[it->second.currentFrame];
                }
            } else {
                previewTextureID = TextureLoader::LoadTexture(button.icon_path);
            }
        }

        if (previewTextureID != 0) {
            ImVec2 previewSize(buttonSizeVec.x * 0.8f, buttonSizeVec.y * 0.8f);
            ImGui::Image((ImTextureID)(intptr_t)previewTextureID, previewSize);
            ImGui::TextUnformatted(button.name.c_str());
        } else {
            ImGui::Text("Moving: %s", button.name.c_str());
        }
        ImGui::EndDragDropSource();
    }
}

InteractionResult GridCellComponent::HandleButtonDropTarget(const std::string& currentButtonId) {
    InteractionResult result{ CellInteractionType::NONE };
     if (ImGui::BeginDragDropTarget()) {
         const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
         if (payload) {
             IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
             const char* droppedButtonIdCStr = (const char*)payload->Data;
             std::string droppedButtonId = droppedButtonIdCStr;

             if (droppedButtonId != currentButtonId) {
                 // Perform the swap directly using ConfigManager reference
                 if (m_configManager.swapButtons(droppedButtonId, currentButtonId)) {
                     std::cout << "[GridCellComponent] Buttons swapped successfully: " << droppedButtonId << " <-> " << currentButtonId << std::endl;
                     result.type = CellInteractionType::DND_COMPLETE;
                     result.layoutChangedByDnd = true;
                 } else {
                      std::cerr << "[GridCellComponent] Failed to swap buttons via ConfigManager." << std::endl;
                      // Result remains NONE
                 }
             } else {
                  std::cout << "[GridCellComponent] Attempted to drop button ID '" << droppedButtonId << "' onto itself. No action taken." << std::endl;
                   // Result remains NONE
             }
         }
         ImGui::EndDragDropTarget();
    }
    return result;
}

InteractionResult GridCellComponent::HandleEmptyCellDropTarget(int page, int row, int col) {
    InteractionResult result{ CellInteractionType::NONE };
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
        if (payload) {
            IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
            const char* droppedButtonIdCStr = (const char*)payload->Data;
            std::string droppedButtonId = droppedButtonIdCStr;

            // Perform the move directly using ConfigManager reference
            if (m_configManager.setButtonPosition(droppedButtonId, page, row, col)) {
                 std::cout << "[GridCellComponent] Button moved successfully: " << droppedButtonId << " -> [" << page << "," << row << "," << col << "]" << std::endl;
                 result.type = CellInteractionType::DND_COMPLETE;
                 result.layoutChangedByDnd = true;
                 result.buttonId = droppedButtonId; // Add buttonId to result
                 result.page = page; // Add location to result
                 result.row = row;
                 result.col = col;
            } else {
                 std::cerr << "[GridCellComponent] Failed to set button position via ConfigManager." << std::endl;
                  // Result remains NONE
            }
        }
        ImGui::EndDragDropTarget();
    }
    return result;
}

InteractionResult GridCellComponent::HandleButtonContextMenu(const std::string& buttonId, int page, int row, int col) {
    InteractionResult result{ CellInteractionType::NONE };
    std::string contextMenuId = "BtnContext_" + buttonId;
    if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
        if (ImGui::MenuItem(m_translator.get("clear_button_label").c_str())) {
            result.type = CellInteractionType::CLEAR_REQUESTED;
            result.buttonId = buttonId;
            result.page = page;
            result.row = row;
            result.col = col;
            // Action (calling ConfigManager) is deferred to the caller (UIButtonGridWindow)
        }
        // Add other menu items here if needed
        ImGui::EndPopup();
    }
    return result;
}

InteractionResult GridCellComponent::HandleEmptyCellContextMenu(int page, int row, int col) {
    InteractionResult result{ CellInteractionType::NONE };
    // Use a consistent naming scheme, maybe based on coordinates
    std::string contextMenuId = "EmptyContext_" + std::to_string(page) + "_" + std::to_string(row) + "_" + std::to_string(col);
    if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
        if (ImGui::MenuItem(m_translator.get("place_existing_button_label").c_str())) {
            result.type = CellInteractionType::PLACE_REQUESTED;
            result.page = page;
            result.row = row;
            result.col = col;
            // Action (opening popup) is deferred to the caller (UIButtonGridWindow)
        }
        // Add other menu items here if needed
        ImGui::EndPopup();
    }
    return result;
} 