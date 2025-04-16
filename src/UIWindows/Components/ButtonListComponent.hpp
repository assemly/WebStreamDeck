#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <functional> // For std::function (callback)
#include <map> // For std::map in CollapsingHeader
#include "../../Managers/ConfigManager.hpp"
#include "../../Managers/TranslationManager.hpp"

// Forward declarations
// class ConfigManager;
// class TranslationManager;

// <<< ADDED: Extern declaration for the global dropped files list >>>
// Defined in main.cpp or another suitable .cpp file
#ifdef _WIN32
extern std::vector<std::wstring> g_DroppedFilesW;
#else
extern std::vector<std::string> g_DroppedFiles;
#endif

// <<< ADDED: Structure for pre-filled data from drag-and-drop >>>
struct PrefilledButtonData {
    std::string suggested_id;
    std::string suggested_name;
    std::string action_type;    // e.g., "launch_app", "open_url"
    std::string action_param;   // e.g., file path, URL
    std::string suggested_icon_path; // Optional icon path
};

class ButtonListComponent {
public:
    // Callback type: void(const std::string& buttonId)
    using EditRequestCallback = std::function<void(const std::string&)>;
    // <<< ADDED: Callback for add request >>>
    using AddRequestCallback = std::function<void(const PrefilledButtonData&)>;

    ButtonListComponent(ConfigManager& configManager, TranslationManager& translator, EditRequestCallback onEditRequested, AddRequestCallback onAddRequested);

    void Draw();

private:
    ConfigManager& m_configManager;
    TranslationManager& m_translator;
    EditRequestCallback m_onEditRequested;
    AddRequestCallback m_onAddRequested; // <<< ADDED: Store the add callback

    // State for delete confirmation
    std::string m_buttonIdToDelete = "";
    bool m_showDeleteConfirmation = false;

    void DrawDeleteConfirmationModal(); // Helper for the modal

    // <<< MODIFIED: Platform-dependent ProcessDroppedFile signature >>>
    #ifdef _WIN32
    void ProcessDroppedFile(const std::wstring& filePathW); // Use wstring on Windows
    #else
    void ProcessDroppedFile(const std::string& filePath);  // Use string otherwise
    #endif
};
