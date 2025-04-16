#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <map>
#include <GL/glew.h> // For GLuint
#include "../../Managers/ConfigManager.hpp"     // Needs ButtonConfig definition
#include "../../Managers/TranslationManager.hpp"
#include "../../Utils/GifLoader.hpp"          // For AnimatedGif struct

// Forward declarations if needed
// class ConfigManager;
// class TranslationManager;

// --- Interaction Result Definition ---

// Possible interaction types that a cell can report back
enum class CellInteractionType {
    NONE,             // No specific interaction occurred
    BUTTON_CLICKED,   // The button within the cell was clicked
    CLEAR_REQUESTED,  // Context menu: "Clear Button" selected
    PLACE_REQUESTED,  // Context menu: "Place Existing Button" selected on empty cell
    DND_COMPLETE      // Drag and drop operation finished *and* potentially updated layout
};

// Structure to hold details about the interaction
struct InteractionResult {
    CellInteractionType type = CellInteractionType::NONE;
    std::string buttonId = "";      // Relevant button ID (e.g., clicked button, button to clear/place)
    int page = -1;                  // Target location for PLACE_REQUESTED
    int row = -1;
    int col = -1;
    bool layoutChangedByDnd = false; // Flag specifically for DND success within the component
};

// --- GridCellComponent Class Definition ---

class GridCellComponent {
public:
    GridCellComponent(ConfigManager& configManager,
                      TranslationManager& translator,
                      std::map<std::string, GifLoader::AnimatedGif>& animatedGifTextures); // Reference to the map

    // Draw a cell containing a button
    InteractionResult DrawButtonCell(const ButtonConfig& button, int page, int row, int col, double currentTime, const ImVec2& buttonSizeVec);

    // Draw an empty cell
    InteractionResult DrawEmptyCell(int page, int row, int col, const ImVec2& buttonSizeVec);

private:
    // Dependencies
    ConfigManager& m_configManager;
    TranslationManager& m_translator;
    std::map<std::string, GifLoader::AnimatedGif>& m_animatedGifTextures; // Store reference

    // Private helper methods (implementations moved from UIButtonGridWindow)
    void DrawSingleButtonVisuals(const ButtonConfig& button, double currentTime, const ImVec2& buttonSizeVec, bool isHovered, bool isActive);
    void HandleButtonDragSource(const ButtonConfig& button, const ImVec2& buttonSizeVec);
    // Drop target might need more context or return value for the caller
    InteractionResult HandleButtonDropTarget(const std::string& currentButtonId);
    InteractionResult HandleEmptyCellDropTarget(int page, int row, int col);
    InteractionResult HandleButtonContextMenu(const std::string& buttonId, int page, int row, int col);
    InteractionResult HandleEmptyCellContextMenu(int page, int row, int col);
}; 