#include "ConfigManager.hpp"
#include <fstream>
#include <iostream> // For error reporting, consider a proper logger later
#include <filesystem> // For checking if file exists

namespace fs = std::filesystem;
using json = nlohmann::json;

ConfigManager::ConfigManager(const std::string& filename) : m_configFilePath(filename)
{
    if (!loadConfig()) {
        std::cerr << "Warning: Failed to load configuration from " << m_configFilePath 
                  << ". Attempting to load/create default configuration." << std::endl;
        loadDefaultConfig();
        std::cout << "Attempting to save default configuration..." << std::endl;
        saveConfig(); 
    }
}

bool ConfigManager::loadConfig()
{
    if (!fs::exists(m_configFilePath)) {
         std::cerr << "Info: Configuration file not found: " << m_configFilePath << std::endl;
         return false; // Indicate failure so constructor loads default
    }

    std::ifstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open configuration file: " << m_configFilePath << std::endl;
        return false;
    }

    try {
        json configJson;
        configFile >> configJson;
        configFile.close(); // Close file after reading

        // Expecting a top-level key, e.g., "buttons", containing an array
        if (configJson.contains("buttons") && configJson["buttons"].is_array()) {
             // Use the safe get_to method for better error handling with the macro
            configJson.at("buttons").get_to(m_buttons);
        } else {
            std::cerr << "Error: Configuration file " << m_configFilePath << " does not contain a 'buttons' array." << std::endl;
            m_buttons.clear(); 
            return false;
        }

        std::cout << "Configuration loaded successfully from " << m_configFilePath << std::endl;
        return true;

    } catch (json::parse_error& e) {
        std::cerr << "Error parsing configuration file: " << m_configFilePath 
                  << "\nMessage: " << e.what() 
                  << "\nException id: " << e.id 
                  << "\nByte position of error: " << e.byte << std::endl;
        m_buttons.clear(); 
        return false;
    } catch (json::exception& e) { // Catch other json exceptions (like type errors)
        std::cerr << "Error processing JSON configuration in " << m_configFilePath << ": " << e.what() << std::endl;
        m_buttons.clear();
        return false;
    } catch (const std::exception& e) {
        std::cerr << "An unknown error occurred while loading the configuration file " << m_configFilePath << ": " << e.what() << std::endl;
        m_buttons.clear();
        return false;
    }
}


bool ConfigManager::saveConfig()
{
    std::ofstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open configuration file for writing: " << m_configFilePath << std::endl;
        return false;
    }

    try {
        // Create a JSON object with a "buttons" key holding the array
        json configJson;
        configJson["buttons"] = m_buttons;

        // Write to file with pretty printing (indentation)
        configFile << configJson.dump(4); // Use 4 spaces for indentation
        configFile.close(); // Close file after writing
        
        std::cout << "Configuration saved successfully to " << m_configFilePath << std::endl;
        return true;

    } catch (json::exception& e) {
        std::cerr << "Error creating JSON or writing configuration file " << m_configFilePath << ": " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
         std::cerr << "An unknown error occurred while saving the configuration file " << m_configFilePath << ": " << e.what() << std::endl;
        return false;
    }
}

const std::vector<ButtonConfig>& ConfigManager::getButtons() const
{
    return m_buttons;
}


std::optional<ButtonConfig> ConfigManager::getButtonById(const std::string& id) const
{
    for (const auto& button : m_buttons) {
        if (button.id == id) {
            return button;
        }
    }
    return std::nullopt;
}

void ConfigManager::loadDefaultConfig()
{
    std::cout << "Loading default button configuration." << std::endl;
    m_buttons = {
        {"btn_notepad", "Notepad", "launch_app", "notepad.exe", ""},
        {"btn_calc", "Calculator", "launch_app", "calc.exe", ""},
        {"btn_google", "Google", "open_url", "https://google.com", ""},
        // Add more default buttons as needed
    };
}

// --- Implementations for modifying methods --- 
// Uncomment and adjust addButton
bool ConfigManager::addButton(const ButtonConfig& button)
{
    // Basic validation: Check for empty ID or Name
    if (button.id.empty() || button.name.empty()) {
        std::cerr << "Error: Cannot add button with empty ID or Name." << std::endl;
        return false;
    }
    // Check for duplicate ID
    for (const auto& existingButton : m_buttons) {
        if (existingButton.id == button.id) {
             std::cerr << "Error: Button with ID '" << button.id << "' already exists." << std::endl;
            return false;
        }
    }
    m_buttons.push_back(button);
    // return saveConfig(); // REMOVED: Do not save immediately
    return true; // Indicate success
}

// Uncomment and adjust updateButton
bool ConfigManager::updateButton(const std::string& id, const ButtonConfig& updatedButton)
{
    // Basic validation for updated button
    if (updatedButton.name.empty()) {
        std::cerr << "Error: Updated button name cannot be empty." << std::endl;
        return false;
    }
    // ID check: Ensure the ID in the updatedButton matches the target id (optional but good practice)
    if (updatedButton.id != id) {
         std::cerr << "Error: Mismatched ID during update attempt (Target: " << id << ", Provided: " << updatedButton.id << "). ID cannot be changed." << std::endl;
         return false;
    }

    for (auto& button : m_buttons) {
        if (button.id == id) {
            // Update fields (except ID)
            button.name = updatedButton.name;
            button.action_type = updatedButton.action_type;
            button.action_param = updatedButton.action_param;
            // button.icon_path = updatedButton.icon_path; // If using icons
            
            // REMOVED: Do not save immediately
            // return saveConfig(); 
            return true; // Indicate success
        }
    }
    std::cerr << "Error: Button with ID '" << id << "' not found for update." << std::endl;
    return false; // Button not found
}

bool ConfigManager::removeButton(const std::string& id)
{
    auto it = std::remove_if(m_buttons.begin(), m_buttons.end(), 
                             [&id](const ButtonConfig& b){ return b.id == id; });
    if (it != m_buttons.end()) {
        m_buttons.erase(it, m_buttons.end());
        return true; // Indicate success
    }
    return false; // Button not found
} 