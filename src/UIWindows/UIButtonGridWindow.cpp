#include "UIButtonGridWindow.hpp"
#include "../Managers/ActionRequestManager.hpp"
#include "../Managers/UIManager.hpp"
#include "../Managers/NetworkManager.hpp"
#include "Components/ButtonSelectorPopupComponent.hpp"
#include "Components/GridCellComponent.hpp"
#include <iostream> // For std::cerr, std::cout
#include <algorithm> // For std::transform and std::max

UIButtonGridWindow::UIButtonGridWindow(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, NetworkManager& networkManager, UIManager& uiManager)
    : m_configManager(configManager), m_actionRequestManager(actionRequestManager), m_translator(translationManager), m_networkManager(networkManager), m_uiManager(uiManager), m_paginationComponent(m_translator), m_buttonSelectorPopup(configManager, translationManager), m_gridCellComponent(configManager, translationManager, networkManager, m_animatedGifTextures) {}

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

    // <<< MODIFIED: Call the rewritten DrawGridCells >>>
    DrawGridCells(layout, *currentPageLayout, currentTime);

    // --- Use the Pagination Component --- (Replaces call to DrawPaginationControls)
    m_paginationComponent.Draw(m_currentPageIndex, layout.page_count, layout.pages); // <<< MODIFIED: Call component's Draw method

    // Draw the button selector popup (it manages its own visibility)
    m_buttonSelectorPopup.Draw(
        // Provide the callback function (lambda) to handle selection
        [this](const std::string& selectedButtonId, int targetPage, int targetRow, int targetCol) {
            std::cout << "[GridWindow] Callback received: Selected '" << selectedButtonId << "' for ["
                      << targetPage << "," << targetRow << "," << targetCol << "]" << std::endl;
            // Attempt to place the button using ConfigManager
            if (m_configManager.setButtonPosition(selectedButtonId, targetPage, targetRow, targetCol)) {
                std::cout << " -> Position set successfully via callback. Notifying UIManager." << std::endl;
                m_uiManager.notifyLayoutChanged(); // Notify WebSocket/UI about the change
            } else {
                std::cerr << " -> Failed to set position via ConfigManager from callback (maybe target occupied?)." << std::endl;
                // Optional: Display an error message to the user via ImGui?
            }
        }
    ); // <<< MODIFIED: Call component's Draw with callback

    ImGui::End();
}

// <<< MODIFIED: Rewritten DrawGridCells to use GridCellComponent >>>
void UIButtonGridWindow::DrawGridCells(const LayoutConfig& layout, const std::vector<std::vector<std::string>>& currentPageLayout, double currentTime) {
    const float button_size = 100.0f;
    ImVec2 buttonSizeVec(button_size, button_size);
    bool overallLayoutChanged = false; // Track if any DND caused a change in this frame

    ImVec2 gridActualStartPos = ImGui::GetCursorScreenPos();

    for (int r = 0; r < layout.rows_per_page; ++r) {
        for (int c = 0; c < layout.cols_per_page; ++c) {
            const std::string& buttonId = currentPageLayout[r][c];
            InteractionResult result{ CellInteractionType::NONE };

            if (!buttonId.empty()) {
                auto buttonOpt = m_configManager.getButtonById(buttonId);
                if (buttonOpt) {
                    // Call the component to draw the button cell and handle interactions
                    result = m_gridCellComponent.DrawButtonCell(*buttonOpt, m_currentPageIndex, r, c, currentTime, buttonSizeVec);
                } else {
                    // Draw Error Placeholder (maybe move this to GridCellComponent too?)
                    ImGui::PushID((std::string("missing_") + buttonId).c_str());
                    ImGui::BeginDisabled(true); ImGui::Button((std::string("ERR:") + buttonId).c_str(), buttonSizeVec); ImGui::EndDisabled();
                    if (ImGui::IsItemHovered()){ ImGui::SetTooltip("Error: Button ID '%s' found in layout but not in configuration.", buttonId.c_str()); }
                    ImGui::PopID();
                }
            } else {
                // Call the component to draw the empty cell and handle interactions
                result = m_gridCellComponent.DrawEmptyCell(m_currentPageIndex, r, c, buttonSizeVec);
            }

            // Process the interaction result from the component
            switch (result.type) {
                case CellInteractionType::BUTTON_CLICKED:
                    if (!result.buttonId.empty()) {
                        m_actionRequestManager.requestAction(result.buttonId);
                    }
                    break;
                case CellInteractionType::CLEAR_REQUESTED:
                    if (m_configManager.clearButtonPosition(result.page, result.row, result.col)) {
                         m_uiManager.notifyLayoutChanged();
                    } else {
                         std::cerr << "[GridWindow] Failed to clear position via ConfigManager for [" << result.page << "," << result.row << "," << result.col << "]" << std::endl;
                    }
                    break;
                case CellInteractionType::PLACE_REQUESTED:
                    m_buttonSelectorPopup.OpenPopup(result.page, result.row, result.col);
                    break;
                case CellInteractionType::DND_COMPLETE:
                    if (result.layoutChangedByDnd) {
                        overallLayoutChanged = true; // Mark that layout changed
                    }
                    break;
                case CellInteractionType::NONE:
                default:
                    // No action needed
                    break;
            }

            // Handle layout: Add SameLine unless it's the last column
            if (c < layout.cols_per_page - 1) {
                ImGui::SameLine();
            }
        } // End column loop
    } // End row loop

    // Draw Grid Lines (code remains the same)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 gridStartPos = gridActualStartPos;
        float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
        float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
        float gridWidth = layout.cols_per_page * button_size + (layout.cols_per_page > 1 ? (layout.cols_per_page - 1) * itemSpacingX : 0);
        float gridHeight = layout.rows_per_page * button_size + (layout.rows_per_page > 1 ? (layout.rows_per_page - 1) * itemSpacingY : 0);
        ImU32 gridColor = ImColor(128, 128, 128, 180);
        float lineThickness = 1.5f;

        for (int c = 1; c < layout.cols_per_page; ++c) {
            float x = gridStartPos.x + c * button_size + (c - 1) * itemSpacingX + itemSpacingX * 0.1f;
            drawList->AddLine(ImVec2(x, gridStartPos.y), ImVec2(x, gridStartPos.y + gridHeight), gridColor, lineThickness);
        }
        for (int r = 1; r < layout.rows_per_page; ++r) {
            float y = gridStartPos.y + r * button_size + (r - 1) * itemSpacingY + itemSpacingY * 0.1f;
            drawList->AddLine(ImVec2(gridStartPos.x, y), ImVec2(gridStartPos.x + gridWidth, y), gridColor, lineThickness);
        }
        ImVec2 gridBottomRight = ImVec2(gridStartPos.x + gridWidth, gridStartPos.y + gridHeight);
        drawList->AddRect(gridStartPos, gridBottomRight, gridColor, 0.0f, ImDrawFlags_None, lineThickness);
    }

    // Notify UIManager ONCE if any DND operation in this frame changed the layout
    if (overallLayoutChanged) {
        m_uiManager.notifyLayoutChanged();
    }
}

// <<< ADDED empty implementation >>>
void UIButtonGridWindow::onLayoutChanged() {
    // TODO: Implement logic if the grid window needs to react to layout changes
    // For example, recalculate grid dimensions or clear caches.
    std::cout << "[UIButtonGridWindow] onLayoutChanged called." << std::endl;
}

// ... DrawAddButtonPopup, DrawContextMenu ...