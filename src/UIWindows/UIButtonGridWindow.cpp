#include "UIButtonGridWindow.hpp"
#include "../Managers/ActionRequestManager.hpp"
#include "../Managers/UIManager.hpp"
#include <iostream> // For std::cerr, std::cout
#include <algorithm> // For std::transform and std::max

UIButtonGridWindow::UIButtonGridWindow(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, UIManager& uiManager)
    : m_configManager(configManager), m_actionRequestManager(actionRequestManager), m_translator(translationManager), m_uiManager(uiManager) {}

UIButtonGridWindow::~UIButtonGridWindow() {
    releaseAnimatedGifTextures();
}

void UIButtonGridWindow::releaseAnimatedGifTextures() {
    for (auto const& [path, gifData] : m_animatedGifTextures) {
        if (gifData.loaded) {
            glDeleteTextures(gifData.frameTextureIds.size(), gifData.frameTextureIds.data());
             // std::cout << "Deleted " << gifData.frameTextureIds.size() << " GIF textures (GridWindow) for: " << path << std::endl;
        }
    }
    m_animatedGifTextures.clear();
}

void UIButtonGridWindow::DrawSingleButton(const ButtonConfig& button, double currentTime, float buttonSize) {
            ImGui::PushID(button.id.c_str());

            ImVec2 topLeft = ImGui::GetCursorScreenPos();
            ImVec2 bottomRight = ImVec2(topLeft.x + buttonSize, topLeft.y + buttonSize);
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // <<< MODIFIED: Use InvisibleButton first for interaction area >>>
            bool buttonClicked = ImGui::InvisibleButton(button.id.c_str(), ImVec2(buttonSize, buttonSize));
            bool isHovered = ImGui::IsItemHovered(); // Get hover state based on InvisibleButton
            bool isActive = ImGui::IsItemActive();   // Get active state (e.g., clicked)

            // Draw background rectangle for the cell
            ImU32 buttonBgColor = ImColor(45, 45, 45, 255); // Dark grey background
            // Optional: Highlight on hover/active?
            if (isActive) {
                buttonBgColor = ImColor(75, 75, 75, 255); 
            } else if (isHovered) {
                buttonBgColor = ImColor(60, 60, 60, 255); 
            }
            float rounding = 4.0f;
            drawList->AddRectFilled(topLeft, bottomRight, buttonBgColor, rounding);
            
            // Define padding and calculate content area
            float padding = 8.0f; 
            float contentSize = buttonSize - 2 * padding;
            ImVec2 contentTopLeft = ImVec2(topLeft.x + padding, topLeft.y + padding);
            if (contentSize < 0) contentSize = 0;
            ImVec2 contentBottomRight = ImVec2(contentTopLeft.x + contentSize, contentTopLeft.y + contentSize);
            // <<< END OF MODIFIED SECTION >>>

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
                    // std::cerr << "Failed to load GIF (GridWindow) for button '" << button.id << "' from path: " << button.icon_path << std::endl;
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

            // --- MODIFIED: Manual Drawing using ImDrawList --- 
            if (useImageButton && textureID != 0) {
                // Draw the image within the padded content area
                drawList->AddImageRounded((ImTextureID)(intptr_t)textureID, 
                                        contentTopLeft,         // Top-left of content area
                                        contentBottomRight,     // Bottom-right of content area
                                        ImVec2(0, 0),           // UV min
                                        ImVec2(1, 1),           // UV max
                                        IM_COL32_WHITE,         // Tint color (white)
                                        0.0f);                  // Rounding for image (optional)
                // We no longer use ImGui::ImageButton
                // We no longer need SetCursorScreenPos or ItemAdd
            } else {
                 // Draw text manually centered in the *content* area
                 ImVec2 textSize = ImGui::CalcTextSize(button.name.c_str());
                 ImVec2 textPos = ImVec2(
                     contentTopLeft.x + (contentSize - textSize.x) * 0.5f,
                     contentTopLeft.y + (contentSize - textSize.y) * 0.5f
                 );
                 if (textPos.x < contentTopLeft.x) textPos.x = contentTopLeft.x;
                 if (textPos.y < contentTopLeft.y) textPos.y = contentTopLeft.y;
                 // Draw centered text within the content area
                 drawList->AddText(textPos, ImColor(255, 255, 255, 255), button.name.c_str());
            }

            // Tooltip - uses the hover state from InvisibleButton
            if (isHovered) {
                ImGui::SetTooltip("%s", button.name.c_str());
            }
            // --- END OF MODIFIED DRAWING SECTION --- 

            // Handle Click - Use ActionRequestManager now
            if (buttonClicked) {
         printf("Button '%s' (ID: %s) clicked! Requesting action...\n",
               button.name.c_str(), button.id.c_str());
        m_actionRequestManager.requestAction(button.id);
    }

    ImGui::PopID();
}

void UIButtonGridWindow::Draw() {
    ImGui::Begin(m_translator.get("button_grid_window_title").c_str());
    double currentTime = ImGui::GetTime();

    // Get layout configuration
    const auto& layout = m_configManager.getLayoutConfig();

    // Check if layout data is valid (basic check)
    if (layout.rows_per_page <= 0 || layout.cols_per_page <= 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid layout configuration (rows/cols <= 0).");
        ImGui::End();
        return;
    }

    // --- Get the layout for the current page --- 
    const std::vector<std::vector<std::string>>* currentPageLayout = nullptr;
    auto pageIt = layout.pages.find(m_currentPageIndex);
    if (pageIt != layout.pages.end()) {
        currentPageLayout = &(pageIt->second);
    } else {
        // Handle case where the current page index is invalid
        if (!layout.pages.empty()) {
            m_currentPageIndex = layout.pages.begin()->first; // Reset to the first available page index
            currentPageLayout = &(layout.pages.begin()->second);
             ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: Page %d not found, showing page %d instead.", pageIt == layout.pages.end() ? m_currentPageIndex /*Original invalid index was m_currentPageIndex before reset*/ : pageIt->first, m_currentPageIndex);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: No pages found in layout configuration.");
            ImGui::End();
            return;
        }
    }

    // Validate the layout data dimensions for the current page
    if (currentPageLayout->size() != layout.rows_per_page) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Layout data row count (%zu) doesn't match configured rows_per_page (%d) for page %d.", currentPageLayout->size(), layout.rows_per_page, m_currentPageIndex);
        ImGui::End();
        return;
    }
    // Optional: Check column consistency within the layout data
    for (size_t r = 0; r < currentPageLayout->size(); ++r) {
         if ((*currentPageLayout)[r].size() != layout.cols_per_page) {
             ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Layout data column count (%zu) at row %zu doesn't match configured cols_per_page (%d) for page %d.", (*currentPageLayout)[r].size(), r, layout.cols_per_page, m_currentPageIndex);
             ImGui::End();
             return;
         }
    }

    // <<< MODIFIED: Call the extracted function to draw grid cells >>>
    DrawGridCells(layout, *currentPageLayout, currentTime);

    // --- Call the extracted function for pagination --- 
    DrawPaginationControls();

    // Call the popup drawing function (it will only draw if needed)
    DrawSelectButtonPopup();

    ImGui::End();
}

// <<< ADDED: Extracted Grid Cell Drawing Logic >>>
void UIButtonGridWindow::DrawGridCells(const LayoutConfig& layout, const std::vector<std::vector<std::string>>& currentPageLayout, double currentTime) {
    const float button_size = 100.0f; // Base button size
    ImVec2 buttonSizeVec(button_size, button_size);
    bool layoutChanged = false; // Flag to track if DND caused a change within this draw cycle

    // <<< ADDED: Get the starting position just before drawing the grid >>>
    ImVec2 gridActualStartPos = ImGui::GetCursorScreenPos(); 

    for (int r = 0; r < layout.rows_per_page; ++r) {
        for (int c = 0; c < layout.cols_per_page; ++c) {
            const std::string& buttonId = currentPageLayout[r][c]; // Direct access after validation in Draw()

            // <<< MODIFIED: Call extracted functions >>>
            if (!buttonId.empty()) {
                DrawButtonInCell(buttonId, r, c, currentTime, buttonSizeVec, layoutChanged);
            } else {
                DrawEmptyCell(r, c, buttonSizeVec, layoutChanged);
            }
            // <<< END OF MODIFICATION >>>

            // Handle layout: Add SameLine unless it's the last column
            if (c < layout.cols_per_page - 1) {
                ImGui::SameLine();
            }
        } // End column loop
    } // End row loop

    // <<< ADDED: Draw Grid Lines >>>
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        // <<< MODIFIED: Use gridActualStartPos captured before the loop >>>
        // ImVec2 windowPos = ImGui::GetWindowPos();
        // ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        // ImVec2 gridStartPos = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 gridStartPos = gridActualStartPos; // Use the recorded start position
        
        float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
        float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
        float gridWidth = layout.cols_per_page * button_size + (layout.cols_per_page > 1 ? (layout.cols_per_page - 1) * itemSpacingX : 0);
        float gridHeight = layout.rows_per_page * button_size + (layout.rows_per_page > 1 ? (layout.rows_per_page - 1) * itemSpacingY : 0);
        // <<< MODIFIED: Brighter color, less transparency, slightly thicker line >>>
        ImU32 gridColor = ImColor(128, 128, 128, 180); // Brighter grey, more opaque
        float lineThickness = 1.5f; // Slightly thicker

        // Draw Vertical Lines
        for (int c = 1; c < layout.cols_per_page; ++c) {
            // Calculate x position relative to the actual start, centered in the spacing
            float x = gridStartPos.x + c * button_size + (c - 1) * itemSpacingX + itemSpacingX * 0.1f; // Reduced offset
            drawList->AddLine(ImVec2(x, gridStartPos.y), ImVec2(x, gridStartPos.y + gridHeight), gridColor, lineThickness);
        }

        // Draw Horizontal Lines
        for (int r = 1; r < layout.rows_per_page; ++r) {
            // Calculate y position relative to the actual start, centered in the spacing
            float y = gridStartPos.y + r * button_size + (r - 1) * itemSpacingY + itemSpacingY * 0.1f; // Reduced offset
            drawList->AddLine(ImVec2(gridStartPos.x, y), ImVec2(gridStartPos.x + gridWidth, y), gridColor, lineThickness);
        }

        // <<< ADDED: Draw Outer Border Rectangle >>>
        ImVec2 gridBottomRight = ImVec2(gridStartPos.x + gridWidth, gridStartPos.y + gridHeight);
        drawList->AddRect(gridStartPos, gridBottomRight, gridColor, 0.0f, ImDrawFlags_None, lineThickness);
        // <<< END OF ADDED CODE >>>
    }
    // <<< END OF GRID LINES CODE >>>

    // Notify if DND caused changes in this cycle
    if (layoutChanged) {
        std::cout << "[GridWindow] Layout changed by DND, notifying UIManager." << std::endl;
        m_uiManager.notifyLayoutChanged();
    }
}

// <<< ADDED: Extracted function for drawing a button in a cell >>>
void UIButtonGridWindow::DrawButtonInCell(const std::string& buttonId, int r, int c, double currentTime, const ImVec2& buttonSizeVec, bool& layoutChanged) {
    auto buttonOpt = m_configManager.getButtonById(buttonId);
    if (buttonOpt) {
        // 1. Draw the button first
        DrawSingleButton(*buttonOpt, currentTime, buttonSizeVec.x);

        // <<< MODIFIED: Call extracted function for Drag Source >>>
        HandleButtonDragSource(*buttonOpt, buttonSizeVec);

        // <<< MODIFIED: Call extracted function for Drop Target >>>
        HandleButtonDropTarget(buttonId, layoutChanged);

        // <<< MODIFIED: Call extracted function for Context Menu >>>
        HandleButtonContextMenu(buttonId, r, c);

    } else { /* Error Placeholder if button data not found */ 
         ImGui::PushID((std::string("missing_") + buttonId).c_str());
         ImGui::BeginDisabled(true); ImGui::Button((std::string("ERR:\n") + buttonId).c_str(), buttonSizeVec); ImGui::EndDisabled();
         if (ImGui::IsItemHovered()){ ImGui::SetTooltip("Error: Button ID '%s' found in layout but not in configuration.", buttonId.c_str()); }
         ImGui::PopID();
    }
}

// <<< ADDED: Extracted function for drawing an empty cell >>>
void UIButtonGridWindow::DrawEmptyCell(int r, int c, const ImVec2& buttonSizeVec, bool& layoutChanged) {
    std::string cellPayloadId = "empty_" + std::to_string(r) + "_" + std::to_string(c);
    ImGui::PushID(cellPayloadId.c_str());
    ImGui::InvisibleButton("empty_cell", buttonSizeVec);
    
    // 1. Context menu for empty cell
    std::string emptyContextMenuId = "EmptyContext_" + std::to_string(r) + "_" + std::to_string(c);
    if (ImGui::BeginPopupContextItem(emptyContextMenuId.c_str())) {
        if (ImGui::MenuItem(m_translator.get("place_existing_button_label").c_str())) { 
            std::cout << "[GridWindow] Place existing button requested at [" << m_currentPageIndex << "," << r << "," << c << "]" << std::endl;
            m_selectTargetPage = m_currentPageIndex;
            m_selectTargetRow = r;
            m_selectTargetCol = c;
            m_openSelectButtonPopup = true;
        }
        ImGui::EndPopup();
    }

    // 2. Drop Target (on empty cell)
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
        if (payload) {
            IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
            const char* droppedButtonIdCStr = (const char*)payload->Data;
            std::string droppedButtonId = droppedButtonIdCStr;
            std::cout << "Drop Event: Target Cell[" << r << "][" << c << "] (Empty), Received Payload ID: '" << droppedButtonId << "'" << std::endl;

            if (m_configManager.setButtonPosition(droppedButtonId, m_currentPageIndex, r, c)) {
                std::cout << " -> Layout updated and saved successfully." << std::endl;
                layoutChanged = true; // Set the flag passed by reference
            } else { std::cerr << " -> Failed to update layout via ConfigManager." << std::endl; }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
}

// <<< ADDED: Extracted Pagination Drawing Logic >>>
void UIButtonGridWindow::DrawPaginationControls() {
    const auto& layout = m_configManager.getLayoutConfig(); // Get layout config again for page_count

    // --- Page Switching UI (if page_count > 1) --- 
    if (layout.page_count > 1) {
        ImGui::Separator(); // Add a separator before the controls
        float buttonWidth = 40.0f;
        float windowWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x; // Get available width

        // Calculate total width needed for buttons and text
        float controlsWidth = buttonWidth * 2.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f + 100.0f; // Approx width for page text
        float startX = (windowWidth - controlsWidth) * 0.5f; // Center the controls
        if (startX < ImGui::GetStyle().WindowPadding.x) startX = ImGui::GetStyle().WindowPadding.x; // Ensure it doesn't overlap padding

        ImGui::SetCursorPosX(startX);

        // Previous Page Button
        if (ImGui::ArrowButton("##PagePrev", ImGuiDir_Left)) {
            if (m_currentPageIndex > 0) { // Check bounds
                 m_currentPageIndex--;
                 // Optional: Check if the new index actually exists in layout.pages map
                 if (layout.pages.find(m_currentPageIndex) == layout.pages.end()) {
                     // Handle case where decrementing skips a page number in the map
                     int originalIndex = m_currentPageIndex + 1;
                     auto it = layout.pages.lower_bound(originalIndex);
                     if (it != layout.pages.begin()) {
                         --it;
                         m_currentPageIndex = it->first;
                     } else {
                         m_currentPageIndex = originalIndex; // Revert decrement if finding prev fails
                     }
                 }
            } else {
                // Optionally wrap around
            }
        }

        ImGui::SameLine();

        // Page Indicator Text
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y); // Align text vertically better
        std::string pageText = std::to_string(m_currentPageIndex + 1) + " / " + std::to_string(layout.page_count);
        float textWidth = ImGui::CalcTextSize(pageText.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (100.0f - textWidth) * 0.5f); // Center text in its allocated space
        ImGui::TextUnformatted(pageText.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y); // Restore Y cursor

        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + buttonWidth + ImGui::GetStyle().ItemSpacing.x + 100.0f + ImGui::GetStyle().ItemSpacing.x); // Position next button

        // Next Page Button
        if (ImGui::ArrowButton("##PageNext", ImGuiDir_Right)) {
            if (m_currentPageIndex < layout.page_count - 1) { // Check bounds
                m_currentPageIndex++;
                // Optional: Check if the new index actually exists in layout.pages map
                if (layout.pages.find(m_currentPageIndex) == layout.pages.end()) {
                     // Handle case where incrementing skips a page number
                     int originalIndex = m_currentPageIndex - 1;
                     auto it = layout.pages.upper_bound(originalIndex);
                     if (it != layout.pages.end()) {
                         m_currentPageIndex = it->first;
                     } else {
                        m_currentPageIndex = originalIndex; // Revert increment if finding next fails
                     }
                }
            } else {
                 // Optionally wrap around
            }
        }
    }
}

// <<< Implementation for the Select Existing Button Popup >>>
void UIButtonGridWindow::DrawSelectButtonPopup() {
    // Use the flag to control opening the popup
    if (m_openSelectButtonPopup) {
        ImGui::OpenPopup(m_translator.get("select_button_popup_title").c_str()); // Need translation key
        m_selectButtonFilter[0] = '\0'; // Clear filter when opening
        m_openSelectButtonPopup = false; // Reset flag after opening request
    }

    // Configure the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Appearing); // Set a reasonable size

    if (ImGui::BeginPopupModal(m_translator.get("select_button_popup_title").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        ImGui::Text(m_translator.get("select_button_popup_desc").c_str(), m_selectTargetPage, m_selectTargetRow, m_selectTargetCol); // Need translation key with placeholders
        ImGui::Separator();

        // --- Filter Input --- 
        ImGui::PushItemWidth(-1); // Use full width
        ImGui::InputTextWithHint("##SelectButtonFilter", m_translator.get("filter_placeholder").c_str(), m_selectButtonFilter, sizeof(m_selectButtonFilter)); // Need translation key
        ImGui::PopItemWidth();
        std::string filterStrLower = m_selectButtonFilter;
        std::transform(filterStrLower.begin(), filterStrLower.end(), filterStrLower.begin(), ::tolower);

        ImGui::Separator();

        // --- Button List --- 
        float listHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 2; // Calculate available height for list
        if (ImGui::BeginChild("ButtonSelectionList", ImVec2(0, std::max(listHeight, 100.0f)), true, ImGuiWindowFlags_HorizontalScrollbar)) {
            const auto& allButtons = m_configManager.getButtons();
            bool selectionMade = false;

            for (const ButtonConfig& button : allButtons) {
                
                // Apply filter (case-insensitive)
                std::string buttonNameLower = button.name;
                std::string buttonIdLower = button.id;
                std::transform(buttonNameLower.begin(), buttonNameLower.end(), buttonNameLower.begin(), ::tolower);
                std::transform(buttonIdLower.begin(), buttonIdLower.end(), buttonIdLower.begin(), ::tolower);

                if (filterStrLower.empty() || 
                    buttonNameLower.find(filterStrLower) != std::string::npos || 
                    buttonIdLower.find(filterStrLower) != std::string::npos)
                {
                    // Display format: "Button Name (ID: button_id)"
                    std::string selectableLabel = button.name + " (ID: " + button.id + ")";
                    if (ImGui::Selectable(selectableLabel.c_str())) {
                        std::cout << "[GridWindow SelectPopup] Selected button '" << button.id << "' for position [" 
                                  << m_selectTargetPage << "," << m_selectTargetRow << "," << m_selectTargetCol << "]" << std::endl;
                        
                        // Attempt to set the button position
                        if (m_configManager.setButtonPosition(button.id, m_selectTargetPage, m_selectTargetRow, m_selectTargetCol)) {
                            std::cout << " -> Position set successfully. Notifying UIManager." << std::endl;
                            m_uiManager.notifyLayoutChanged(); // Notify UI/Websocket
                            selectionMade = true;
                            ImGui::CloseCurrentPopup(); // Close popup on successful selection
                        } else {
                            std::cerr << " -> Failed to set position via ConfigManager (maybe target occupied?)." << std::endl;
                            // TODO: Show error message in the popup?
                        }
                    }
                }
            }
            // Early exit if selection was made inside the loop to avoid issues with closing popup
            if (selectionMade) {
                 ImGui::EndChild();
                 ImGui::EndPopup();
                 return; // Exit function early
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        // --- Action Buttons --- 
        if (ImGui::Button(m_translator.get("cancel_button_label").c_str(), ImVec2(120, 0))) { // Need translation key
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// <<< ADDED: Extracted function for handling button drag source >>>
void UIButtonGridWindow::HandleButtonDragSource(const ButtonConfig& button, const ImVec2& buttonSizeVec) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        const char* idPayload = button.id.c_str(); // Use button directly
        // Assuming r, c are not needed here, but if they were, they'd need to be passed in.
        // std::cout << "Drag Start: ButtonID: '" << button.id << "', Payload to set: '" << idPayload << "'" << std::endl;
        ImGui::SetDragDropPayload("BUTTON_GRID_ITEM", idPayload, strlen(idPayload) + 1);
        
        // Draw preview
        GLuint previewTextureID = 0;
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

// <<< ADDED: Extracted function for handling button drop target >>>
void UIButtonGridWindow::HandleButtonDropTarget(const std::string& currentButtonId, bool& layoutChanged) {
    if (ImGui::BeginDragDropTarget()) {
         const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
         if (payload) {
             IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
             const char* droppedButtonIdCStr = (const char*)payload->Data;
             std::string droppedButtonId = droppedButtonIdCStr;
             // Assuming r, c are not needed here, but if they were, they'd need to be passed in.
             // std::cout << "Drop Event: Target Button ID: '" << currentButtonId << "', Received Payload ID: '" << droppedButtonId << "'" << std::endl;

             if (droppedButtonId != currentButtonId) { // Prevent self-drop
                 if (m_configManager.swapButtons(droppedButtonId, currentButtonId)) {
                     std::cout << " -> Buttons swapped and saved successfully." << std::endl;
                     layoutChanged = true; // Set the flag passed by reference
                 } else { 
                      std::cerr << " -> Failed to swap buttons via ConfigManager." << std::endl; 
                 }
             } else { 
                  std::cout << "Attempted to drop button ID '" << droppedButtonId << "' onto itself. No action taken." << std::endl; 
             }
         }
         ImGui::EndDragDropTarget();
    }
}

// <<< ADDED: Extracted function for handling button context menu >>>
void UIButtonGridWindow::HandleButtonContextMenu(const std::string& buttonId, int r, int c) {
    std::string contextMenuId = "ContextMenu_" + buttonId;
    if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) { 
        if (ImGui::MenuItem(m_translator.get("clear_button_label").c_str())) {
            std::cout << "[GridWindow] Clear position requested for button: " << buttonId << " at [" << m_currentPageIndex << "," << r << "," << c << "]" << std::endl;
            if (m_configManager.clearButtonPosition(m_currentPageIndex, r, c)) { 
                std::cout << " -> Position cleared successfully. Notifying UIManager." << std::endl;
                m_uiManager.notifyLayoutChanged(); // Directly notify here
            } else {
                std::cerr << " -> Failed to clear position via ConfigManager." << std::endl;
            }
        }
        ImGui::EndPopup();
    }
}