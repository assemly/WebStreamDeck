#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <optional>
#include "../Services/ActionExecutionService.hpp"

// Forward declaration for ConfigManager
class ConfigManager;

class ActionRequestManager {
public:
    // Constructor now takes ConfigManager, creates Service internally
    explicit ActionRequestManager(ConfigManager& configManager);

    // Call this from any thread to request an action
    void requestAction(const std::string& buttonId);

    // Call this from the main thread to process all queued actions
    void processPendingActions();

    // std::optional<std::string> getNextActionToProcess(); // <<< REMOVED, logic moved to processPendingActions

private:
    // ActionExecutionService& m_executionService; // REMOVED reference
    ConfigManager& m_configManager;           // Reference to the config manager
    ActionExecutionService m_executionService; // <<< ADDED: Owns the service instance
    std::queue<std::string> m_actionQueue;
    std::mutex m_queueMutex;
}; 