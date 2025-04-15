#pragma once

#include <imgui.h>
#include <string>
#include <map>
#include "ConfigManager.hpp"
#include "ActionRequestManager.hpp"
#include "TranslationManager.hpp"
#include "../UIWindows/UIButtonGridWindow.hpp"
#include "../UIWindows/UIConfigurationWindow.hpp"
#include "../UIWindows/UIStatusLogWindow.hpp"
#include "../UIWindows/UIQrCodeWindow.hpp"


class UIManager
{
public:
    explicit UIManager(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager);
    ~UIManager();

    void drawUI();

    // Method to receive server status from main application
    void setServerStatus(bool isRunning, int port);

private:

    // Server status variables
    bool m_isServerRunning = false;
    int m_serverPort = 0;
    std::string m_serverIP = "Fetching..."; // Default value
 
    // Member variables for the sub-windows
    UIButtonGridWindow m_buttonGridWindow;
    UIConfigurationWindow m_configWindow;
    UIStatusLogWindow m_statusLogWindow;
    UIQrCodeWindow m_qrCodeWindow;
}; 