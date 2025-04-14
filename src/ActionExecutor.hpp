#pragma once

#include <string>
#include "ConfigManager.hpp" // Include ConfigManager to access button configs

class ActionExecutor
{
public:
    // Constructor takes a reference to ConfigManager
    explicit ActionExecutor(ConfigManager& configManager);

    // Executes the action associated with the given button ID
    bool executeAction(const std::string& buttonId);

private:
    ConfigManager& m_configManager; // Store a reference to access config

    // Private helper methods for specific action types (optional)
    bool executeLaunchApp(const std::string& path);
    bool executeOpenUrl(const std::string& url);
    bool executeHotkey(const std::string& keys);
    // bool executeSystemCommand(const std::string& command); // Add later
}; 