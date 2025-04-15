#include "ActionRequestManager.hpp"
#include "../Services/ActionExecutionService.hpp" // Included via header now
#include "ConfigManager.hpp"       // Need full definition for calling getButtonById
#include <iostream> // For debug logging

// Constructor implementation - initializes ConfigManager ref, default constructs Service member
ActionRequestManager::ActionRequestManager(ConfigManager& configManager)
    : m_configManager(configManager)
      // m_executionService is default-constructed
{}

// Call this from any thread to request an action
void ActionRequestManager::requestAction(const std::string& buttonId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_actionQueue.push(buttonId);
    // Optional: Add logging if helpful
    // std::cout << "[Req Mgr] Queued action request for button ID: " << buttonId << std::endl;
}

// Call this from the main thread to process all queued actions
void ActionRequestManager::processPendingActions() {
    // Process all actions currently in the queue in this iteration
    while (true) {
        std::string buttonIdToProcess;
        bool actionFound = false;

        // Quickly check queue and get ID if available
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_actionQueue.empty()) {
                buttonIdToProcess = m_actionQueue.front();
                m_actionQueue.pop();
                actionFound = true;
            }
        }

        // If no action was found in the queue, break the loop
        if (!actionFound) {
            break; 
        }

        // --- Coordination Logic (Now inside the Manager) ---
        // Get configuration for the button ID
        auto buttonConfigOpt = m_configManager.getButtonById(buttonIdToProcess);
        if (!buttonConfigOpt) {
            std::cerr << "[Req Mgr] Error: Button with ID '" << buttonIdToProcess << "' not found in config." << std::endl;
            continue; // Skip to the next action in the queue
        }

        const ButtonConfig& config = *buttonConfigOpt;
        const std::string& actionType = config.action_type;
        const std::string& actionParam = config.action_param;

        // Call the MEMBER execution service
        // std::cout << "[Req Mgr] Processing action for button '" << buttonIdToProcess << "': Type='" << actionType << "', Param='" << actionParam << "'" << std::endl;
        try {
            m_executionService.executeAction(actionType, actionParam);
        } catch (const std::exception& e) {
            std::cerr << "[Req Mgr] Exception during action execution for button '" << buttonIdToProcess << "': " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[Req Mgr] Unknown exception during action execution for button '" << buttonIdToProcess << "'" << std::endl;
        }
        // --- End Coordination Logic ---
    }
} 