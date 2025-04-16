#pragma once

#include <imgui.h>
#include <string>
#include <map>
#include "ConfigManager.hpp"
#include "ActionRequestManager.hpp"
#include "TranslationManager.hpp"
#include "NetworkManager.hpp"
#include "../UIWindows/UIButtonGridWindow.hpp"
#include "../UIWindows/UIConfigurationWindow.hpp"
#include "../UIWindows/UIStatusLogWindow.hpp"
#include "../UIWindows/UIQrCodeWindow.hpp"
#include "../UIWindows/UILayoutSettingsWindow.hpp"
#include "../UIWindows/UIAboutWindow.hpp"
#include <vector>


class UIManager
{
public:
    UIManager(ConfigManager& configManager, ActionRequestManager& actionRequestManager, TranslationManager& translationManager, NetworkManager& networkManager);
    ~UIManager();

    void drawUI();

    // Method to receive server status from main application
    void setServerStatus(bool isRunning, int port);

    void notifyLayoutChanged();

    // Method to process dropped files
    #ifdef _WIN32
    void ProcessDroppedFiles(const std::vector<std::wstring>& files);
    #else
    void ProcessDroppedFiles(const std::vector<std::string>& files);
    #endif

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
    UILayoutSettingsWindow m_layoutSettingsWindow;
    UIAboutWindow m_aboutWindow;

    NetworkManager& m_networkManager;
    TranslationManager& m_translationManager;

    bool m_firstFrame = true; // Flag for initial focus setting

    // Configure the uWS::App with routes from both servers and start listening
    void configure_and_listen(int port);
}; 