#pragma once

#include <string>
#include "ConfigManager.hpp" // Include ConfigManager to access button configs
#include <queue>      // For std::queue
#include <mutex>      // For std::mutex

class ActionExecutor
{
public:
    // Constructor takes a reference to ConfigManager
    explicit ActionExecutor(ConfigManager& configManager);

    // Renamed: Now queues the action request from any thread
    void requestAction(const std::string& buttonId);

    // ADDED: Processes pending actions on the calling thread (should be main thread)
    void processPendingActions();

private:
    ConfigManager& m_configManager; // Store a reference to access config
    
    // ADDED: Thread-safe queue for action requests
    std::queue<std::string> m_actionQueue;
    std::mutex m_queueMutex;

    // ADDED: The actual execution logic, called by processPendingActions
    void executeActionInternal(const std::string& buttonId);

    // Private helper methods for specific action types (optional)
    bool executeLaunchApp(const std::string& path);
    bool executeOpenUrl(const std::string& url);
    bool executeHotkey(const std::string& keys);
    // bool executeSystemCommand(const std::string& command); // Add later
}; 