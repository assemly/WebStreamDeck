#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <map>
#include <GL/glew.h> // For GLuint
#include "../Managers/ConfigManager.hpp"
#include "../Managers/TranslationManager.hpp"
#include "../Managers/ActionRequestManager.hpp"
#include "../Utils/GifLoader.hpp" // For AnimatedGif struct
#include "../Utils/TextureLoader.hpp"

// Forward declare UIManager to avoid circular dependency if needed later
class UIManager;

class UIButtonGridWindow {
public:
    UIButtonGridWindow(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, UIManager& uiManager);
    ~UIButtonGridWindow(); // Destructor to manage resources if needed

    void Draw();

    // Helper function to load static textures (moved from UIManager)
    // GLuint LoadTextureFromFile(const char* filename);

private:
    ConfigManager& m_configManager;
    ActionRequestManager& m_actionRequestManager;
    TranslationManager& m_translator;
    UIManager& m_uiManager;

    // Texture management moved here from UIManager for icons used in this window
    // std::map<std::string, GLuint> m_buttonIconTextures;
    std::map<std::string, GifLoader::AnimatedGif> m_animatedGifTextures;

    // <<< ADDED: Member variable to store the current page index >>>
    int m_currentPageIndex = 0; // Default to page 0

    // <<< MODIFIED: Variables and functions for selecting an existing button >>>
    bool m_openSelectButtonPopup = false;
    int m_selectTargetPage = 0;
    int m_selectTargetRow = 0;
    int m_selectTargetCol = 0;
    void DrawSelectButtonPopup();

    // <<< ADDED: Filter text for the select button popup >>>
    char m_selectButtonFilter[128] = "";

    // <<< ADDED: Declaration for extracted pagination drawing logic >>>
    void DrawPaginationControls();

    // <<< ADDED: Declaration for extracted grid cell drawing logic >>>
    void DrawGridCells(const LayoutConfig& layout, const std::vector<std::vector<std::string>>& currentPageLayout, double currentTime);

    // <<< ADDED: Declarations for extracted cell content drawing logic >>>
    void DrawButtonInCell(const std::string& buttonId, int r, int c, double currentTime, const ImVec2& buttonSizeVec, bool& layoutChanged);
    void DrawEmptyCell(int r, int c, const ImVec2& buttonSizeVec, bool& layoutChanged);

    // <<< ADDED: Declarations for extracted button interaction logic >>>
    void HandleButtonDragSource(const ButtonConfig& button, const ImVec2& buttonSizeVec);
    void HandleButtonDropTarget(const std::string& currentButtonId, bool& layoutChanged);
    void HandleButtonContextMenu(const std::string& buttonId, int r, int c);


    void releaseAnimatedGifTextures(); // Helper for GIF textures

    // <<< ADDED: Helper function to draw a single button
    void DrawSingleButton(const ButtonConfig& button, double currentTime, float buttonSize);
    
    // <<< ADDED: Helper for File Dialogs specific to this window (if needed) >>>
    // void HandleAddButtonFileDialog();
};