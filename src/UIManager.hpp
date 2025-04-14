#pragma once

#include <imgui.h>
#include <string>
#include <map>
#include "ConfigManager.hpp"
#include "ActionExecutor.hpp"
#include "TranslationManager.hpp"
#include "UIWindows/UIButtonGridWindow.hpp"
#include "UIWindows/UIConfigurationWindow.hpp"
#include "UIWindows/UIStatusLogWindow.hpp"
#include "UIWindows/UIQrCodeWindow.hpp"
#include "Utils/NetworkUtils.hpp"


class UIManager
{
public:
    explicit UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager);
    ~UIManager();

    void drawUI();

    // Method to receive server status from main application
    void setServerStatus(bool isRunning, int port);

private:
    ConfigManager& m_configManager;
    ActionExecutor& m_actionExecutor;
    TranslationManager& m_translator;

    // Server status variables
    bool m_isServerRunning = false;
    int m_serverPort = 0;
    std::string m_serverIP = "Fetching..."; // Default value

    
    // <<< ADDED: Member variable for the button grid window
    UIButtonGridWindow m_buttonGridWindow;
    // <<< ADDED: Member variable for the configuration window
    UIConfigurationWindow m_configWindow;
    // <<< ADDED: Member variable for the status log window
    UIStatusLogWindow m_statusLogWindow;
    // <<< ADDED: Member variable for the QR code window
    UIQrCodeWindow m_qrCodeWindow;
}; 