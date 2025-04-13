#pragma once

#include <imgui.h>
#include "ConfigManager.hpp"
#include "ActionExecutor.hpp"

class UIManager
{
public:
    explicit UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor);
    ~UIManager();

    void drawUI();

private:
    ConfigManager& m_configManager;
    ActionExecutor& m_actionExecutor;
    // Private helper methods for drawing specific windows
    void drawButtonGridWindow();
    void drawConfigurationWindow();
    void drawStatusLogWindow();
    void drawQrCodeWindow();

    // Add any UI state variables here if needed later
    // bool show_demo_window = false; 
}; 