#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <optional>
#include "../Services/ActionExecutionService.hpp"
#include "../Services/SoundPlaybackService.hpp"

// Forward declaration for ConfigManager & SoundPlaybackService
class ConfigManager;
// class SoundPlaybackService; // No longer needed as full header included

class ActionRequestManager {
public:
    // Constructor now takes ConfigManager and SoundPlaybackService
    explicit ActionRequestManager(ConfigManager& configManager, SoundPlaybackService& soundService);

    // Call this from any thread to request an action
    void requestAction(const std::string& buttonId);

    // Call this from the main thread to process all queued actions
    void processPendingActions();

    // std::optional<std::string> getNextActionToProcess(); // <<< REMOVED, logic moved to processPendingActions

private:
    // ActionExecutionService& m_executionService; // REMOVED reference
    ConfigManager& m_configManager;           // Reference to the config manager
    ActionExecutionService m_executionService; // Owns the service instance
    std::queue<std::string> m_actionQueue;
    std::mutex m_queueMutex;
}; 