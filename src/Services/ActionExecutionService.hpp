#pragma once

#include <string>
#include "SoundPlaybackService.hpp"
// No longer needs ConfigManager
// Include necessary headers for Utils if helpers need them, e.g.:
// #include "../Utils/InputUtils.hpp"

// Forward declarations if needed

class ActionExecutionService {
public:
    // Constructor now takes SoundPlaybackService reference
    explicit ActionExecutionService(SoundPlaybackService& soundService); 

    // Executes an action based on explicit type and parameter
    void executeAction(const std::string& actionType, const std::string& actionParam);

private:
    SoundPlaybackService& m_soundService;

    // Private helper methods for specific action types (can remain here)
    bool executeLaunchApp(const std::string& path);
    bool executeOpenUrl(const std::string& url);
    bool executeHotkey(const std::string& keys);
    // ... potentially other helpers ...
}; 