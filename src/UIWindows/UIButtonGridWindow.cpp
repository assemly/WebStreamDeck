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

            // Button Rendering
            bool buttonClicked = false;
    ImVec2 sizeVec(buttonSize, buttonSize);
            if (useImageButton && textureID != 0) {
                 buttonClicked = ImGui::ImageButton(button.id.c_str(),
                                                    (ImTextureID)(intptr_t)textureID,
                                            sizeVec,
                                                    ImVec2(0, 0),
                                                    ImVec2(1, 1),
                                            ImVec4(0,0,0,0), 
                                            ImVec4(1,1,1,1));
                 if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", button.name.c_str());
                 }
            } else {
        buttonClicked = ImGui::Button(button.name.c_str(), sizeVec);
            }

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

    // --- Draw the Grid based on Layout --- 
    const float button_size = 100.0f; // Base button size (can be dynamic later)
    ImVec2 buttonSizeVec(button_size, button_size);
    bool layoutChanged = false; // Flag to track if DND caused a change

    for (int r = 0; r < layout.rows_per_page; ++r) {
        for (int c = 0; c < layout.cols_per_page; ++c) {
            const std::string& buttonId = (*currentPageLayout)[r][c];

            if (!buttonId.empty()) {
                // --- Existing Button --- 
                auto buttonOpt = m_configManager.getButtonById(buttonId);
                if (buttonOpt) {
                    // 1. Draw the button first (which pushes/pops its own ID)
                    DrawSingleButton(*buttonOpt, currentTime, buttonSizeVec.x);

                    // 2. Check if the button JUST DRAWN is being dragged (Source)
                    //    This associates the DND source directly with the button widget.
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                        const char* idPayload = buttonId.c_str();
                         std::cout << "Drag Start: Cell[" << r << "][" << c << "], ButtonID: '" << buttonId << "', Payload to set: '" << idPayload << "'" << std::endl; // Keep log
                        ImGui::SetDragDropPayload("BUTTON_GRID_ITEM", idPayload, strlen(idPayload) + 1);
                        
                        // Draw preview (same as before)
                        GLuint previewTextureID = 0;
                        std::string lowerIconPath = buttonOpt->icon_path;
                        std::transform(lowerIconPath.begin(), lowerIconPath.end(), lowerIconPath.begin(), ::tolower);
                        if (!buttonOpt->icon_path.empty()) {
                             if (lowerIconPath.length() > 4 && lowerIconPath.substr(lowerIconPath.length() - 4) == ".gif") {
                                auto it = m_animatedGifTextures.find(buttonOpt->icon_path);
                                if (it != m_animatedGifTextures.end() && it->second.loaded && !it->second.frameTextureIds.empty()) {
                                    // Use the *current* frame of the GIF for the preview
                                    previewTextureID = it->second.frameTextureIds[it->second.currentFrame]; 
                                }
                            } else {
                                previewTextureID = TextureLoader::LoadTexture(buttonOpt->icon_path); 
                            }
                        }
                        if (previewTextureID != 0) {
                            ImVec2 previewSize(buttonSizeVec.x * 0.8f, buttonSizeVec.y * 0.8f); // Slightly smaller preview
                            ImGui::Image((ImTextureID)(intptr_t)previewTextureID, previewSize);
                            // Optional: Add button name text below image
                            ImGui::TextUnformatted(buttonOpt->name.c_str()); 
                        } else {
                            ImGui::Text("Moving: %s", buttonOpt->name.c_str()); // Fallback text
                        }

                        ImGui::EndDragDropSource();
                    }

                    // 3. Make the button JUST DRAWN also a Drop Target
                    if (ImGui::BeginDragDropTarget()) {
                         const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
                         if (payload) {
                             IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
                             const char* droppedButtonIdCStr = (const char*)payload->Data;
                             std::string droppedButtonId = droppedButtonIdCStr;
                             std::cout << "Drop Event: Target Cell[" << r << "][" << c << "], Target Button ID: '" << buttonId << "', Received Payload ID: '" << droppedButtonId << "'" << std::endl; // Updated log

                             if (droppedButtonId != buttonId) { // Prevent self-drop
                                 if (m_configManager.swapButtons(droppedButtonId, buttonId)) {
                                     std::cout << " -> Buttons swapped and saved successfully." << std::endl;
                                     layoutChanged = true;
                                     // WebSocket update will be triggered by the flag later
                                 } else { 
                                      std::cerr << " -> Failed to swap buttons via ConfigManager." << std::endl; 
                                 }
                             } else { 
                                  std::cout << "Attempted to drop button ID '" << droppedButtonId << "' onto itself. No action taken." << std::endl; 
                             }
                         }
                         ImGui::EndDragDropTarget();
                    }

                    // <<< ADDED: Right-click context menu for deleting >>>
                    std::string contextMenuId = "ContextMenu_" + buttonId;
                    if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) { // Use unique ID for context menu
                        if (ImGui::MenuItem(m_translator.get("clear_button_label").c_str())) {
                            std::cout << "[GridWindow] Clear position requested for button: " << buttonId << " at [" << m_currentPageIndex << "," << r << "," << c << "]" << std::endl;
                            // Call ConfigManager to clear the button's position
                            if (m_configManager.clearButtonPosition(m_currentPageIndex, r, c)) { // Pass current page, row, col
                                std::cout << " -> Position cleared successfully. Notifying UIManager." << std::endl;
                                m_uiManager.notifyLayoutChanged(); // Notify to update UI and broadcast
                            } else {
                                std::cerr << " -> Failed to clear position via ConfigManager." << std::endl;
                                // Optionally show an error message to the user here?
                            }
                        }
                        ImGui::EndPopup();
                    }
                    // <<< END OF ADDED CODE >>>

                } else { /* Error Placeholder */ 
                     ImGui::PushID((std::string("missing_") + buttonId).c_str()); // Need ID for placeholder
                     ImGui::BeginDisabled(true); ImGui::Button((std::string("ERR:\n") + buttonId).c_str(), buttonSizeVec); ImGui::EndDisabled();
                     if (ImGui::IsItemHovered()){ ImGui::SetTooltip("Error: Button ID '%s' found in layout but not in configuration.", buttonId.c_str()); }
                     ImGui::PopID();
                }
            } else {
                // --- Empty Slot: Still needs its own ID and Drop Target --- 
                std::string cellPayloadId = "empty_" + std::to_string(r) + "_" + std::to_string(c);
                ImGui::PushID(cellPayloadId.c_str());
                ImGui::InvisibleButton("empty_cell", buttonSizeVec);
                
                // <<< ADDED: Context menu for empty cell >>>
                std::string emptyContextMenuId = "EmptyContext_" + std::to_string(r) + "_" + std::to_string(c);
                if (ImGui::BeginPopupContextItem(emptyContextMenuId.c_str())) {
                    if (ImGui::MenuItem(m_translator.get("place_existing_button_label").c_str())) { // Need translation key
                        std::cout << "[GridWindow] Place existing button requested at [" << m_currentPageIndex << "," << r << "," << c << "]" << std::endl;
                        // Store target location for the select popup
                        m_selectTargetPage = m_currentPageIndex;
                        m_selectTargetRow = r;
                        m_selectTargetCol = c;
                        m_openSelectButtonPopup = true; // Trigger the *select* popup
                    }
                    ImGui::EndPopup();
                }
                // <<< END OF ADDED CODE >>>

                // Make empty cell a drop target
                if (ImGui::BeginDragDropTarget()) {
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BUTTON_GRID_ITEM");
                    if (payload) {
                        IM_ASSERT(payload->DataSize == strlen((const char*)payload->Data) + 1);
                        const char* droppedButtonIdCStr = (const char*)payload->Data;
                        std::string droppedButtonId = droppedButtonIdCStr;
                        std::cout << "Drop Event: Target Cell[" << r << "][" << c << "] (Empty), Received Payload ID: '" << droppedButtonId << "'" << std::endl;

                        // Dropping onto empty slot - Use setButtonPosition
                        if (m_configManager.setButtonPosition(droppedButtonId, m_currentPageIndex, r, c)) {
                            std::cout << " -> Layout updated and saved successfully." << std::endl;
                            layoutChanged = true;
                        } else { std::cerr << " -> Failed to update layout via ConfigManager." << std::endl; }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();
            }

            // Handle layout: Add SameLine unless it's the last column
            if (c < layout.cols_per_page - 1) {
                ImGui::SameLine();
            }
        } // End column loop
    } // End row loop

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
                 // If not, maybe loop around or find the previous existing key?
                 // For simplicity now, just decrement.
                 if (layout.pages.find(m_currentPageIndex) == layout.pages.end()) {
                     // Handle case where decrementing skips a page number in the map
                     // Find the largest key smaller than the original m_currentPageIndex
                     int originalIndex = m_currentPageIndex + 1;
                     auto it = layout.pages.lower_bound(originalIndex);
                     if (it != layout.pages.begin()) {
                         --it;
                         m_currentPageIndex = it->first;
                     } else {
                         // If already at the beginning or no smaller key, maybe loop to end?
                         // Or just stay at 0 if it exists? For now, revert if no prev found easily.
                         m_currentPageIndex = originalIndex; // Revert decrement if finding prev is complex/fails
                     }
                 }
            } else {
                // Optionally wrap around to the last page
                // m_currentPageIndex = layout.page_count - 1; 
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
                     // Handle case where incrementing skips a page number in the map
                     // Find the smallest key larger than the original m_currentPageIndex
                     int originalIndex = m_currentPageIndex - 1;
                     auto it = layout.pages.upper_bound(originalIndex);
                     if (it != layout.pages.end()) {
                         m_currentPageIndex = it->first;
                     } else {
                        // If already at the end or no larger key, maybe loop to start?
                        // Or just stay at max if it exists? For now, revert if no next found easily.
                        m_currentPageIndex = originalIndex; // Revert increment if finding next fails
                     }
                }
            } else {
                 // Optionally wrap around to the first page
                // m_currentPageIndex = 0;
            }
        }
    }

    // --- Notify UIManager if layout changed --- 
    if (layoutChanged) {
        std::cout << "[GridWindow] Layout changed by DND, notifying UIManager." << std::endl;
        m_uiManager.notifyLayoutChanged();
    }

    // Call the popup drawing function (it will only draw if needed)
    DrawSelectButtonPopup();

    ImGui::End();
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