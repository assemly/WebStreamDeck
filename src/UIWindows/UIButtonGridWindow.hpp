#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <map>
#include <GL/glew.h> // For GLuint
#include "../Managers/ConfigManager.hpp"
#include "../Managers/TranslationManager.hpp"
#include "../Managers/ActionRequestManager.hpp"
#include "../Managers/NetworkManager.hpp"
#include "../Utils/GifLoader.hpp" // For AnimatedGif struct
#include "../Utils/TextureLoader.hpp"
#include "Components/GridPaginationComponent.hpp"
#include "Components/ButtonSelectorPopupComponent.hpp"
#include "Components/GridCellComponent.hpp"

// Forward declare UIManager to avoid circular dependency if needed later
class UIManager;
// <<< ADDED: Forward declare non-namespaced TranslationManager >>>
class TranslationManager; // Add this if TranslationManager.hpp isn't included

class UIButtonGridWindow {
public:
    UIButtonGridWindow(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, NetworkManager& networkManager, UIManager& uiManager);
    ~UIButtonGridWindow(); // Destructor to manage resources if needed

    void Draw();
    void onLayoutChanged(); // <<< ADDED declaration

    // Helper function to load static textures (moved from UIManager)
    // GLuint LoadTextureFromFile(const char* filename);

private:
    ConfigManager& m_configManager;
    ActionRequestManager& m_actionRequestManager;
    // <<< MODIFIED: Use non-namespaced type for member variable >>>
    TranslationManager& m_translator;
    UIManager& m_uiManager;
    NetworkManager& m_networkManager;
    // Texture management moved here from UIManager for icons used in this window
    // std::map<std::string, GLuint> m_buttonIconTextures;
    std::map<std::string, GifLoader::AnimatedGif> m_animatedGifTextures;

    // <<< ADDED: Member variable to store the current page index >>>
    int m_currentPageIndex = 0; // Default to page 0

    // <<< ADDED: Component Instances >>>
    GridPaginationComponent m_paginationComponent;
    ButtonSelectorPopupComponent m_buttonSelectorPopup;
    GridCellComponent m_gridCellComponent;

    void releaseAnimatedGifTextures(); // Helper for GIF textures

    // <<< ADDED: Helper function to draw a single button
    // void DrawSingleButton(const ButtonConfig& button, double currentTime, float buttonSize);
    
    // <<< ADDED: Helper for File Dialogs specific to this window (if needed) >>>
    // void HandleAddButtonFileDialog();

    // <<< ADDED: Declaration for extracted pagination drawing logic >>>
    // void DrawPaginationControls();

    // <<< ADDED: Declaration for extracted grid cell drawing logic >>>
    void DrawGridCells(const LayoutConfig& layout, const std::vector<std::vector<std::string>>& currentPageLayout, double currentTime);

    // <<< ADDED: Declarations for extracted cell content drawing logic >>>
    // void DrawButtonInCell(const std::string& buttonId, int r, int c, double currentTime, const ImVec2& buttonSizeVec, bool& layoutChanged);
    // void DrawEmptyCell(int r, int c, const ImVec2& buttonSizeVec, bool& layoutChanged);

    // <<< ADDED: Declarations for extracted button interaction logic >>>
    // void HandleButtonDragSource(const ButtonConfig& button, const ImVec2& buttonSizeVec);
    // void HandleButtonDropTarget(const std::string& currentButtonId, bool& layoutChanged);
    // void HandleButtonContextMenu(const std::string& buttonId, int r, int c);

    void DrawAddButtonPopup();
    void DrawContextMenu();
};