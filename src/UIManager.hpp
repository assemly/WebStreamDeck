#pragma once

#include <imgui.h>

class UIManager
{
public:
    UIManager();
    ~UIManager();

    void drawUI();

private:
    // Private helper methods for drawing specific windows
    void drawButtonGridWindow();
    void drawConfigurationWindow();
    void drawStatusLogWindow();
    void drawQrCodeWindow();

    // Add any UI state variables here if needed later
    // bool show_demo_window = false; 
}; 