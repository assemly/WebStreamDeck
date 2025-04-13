#pragma once

#include <string>
#include <vector>
#include <optional> // Include for optional return type
#include <nlohmann/json.hpp>

// Define structure for a single button configuration
struct ButtonConfig {
    std::string id = ""; // Provide default values
    std::string name = "";
    std::string action_type = ""; // e.g., "launch_app", "hotkey", "open_url"
    std::string action_param = ""; // e.g., "notepad.exe", "CTRL+ALT+T", "https://google.com"
    std::string icon_path = ""; // Optional path to an icon file

    // Add functions for JSON serialization/deserialization (using nlohmann/json)
    // Using NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT for robustness against missing fields
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ButtonConfig, id, name, action_type, action_param, icon_path);
};

class ConfigManager
{
public:
    explicit ConfigManager(const std::string& filename = "config.json");

    // Load configuration from the specified file
    bool loadConfig();

    // Save current configuration to the specified file
    bool saveConfig();

    // Get all button configurations
    const std::vector<ButtonConfig>& getButtons() const;

    // Get a specific button configuration by ID
    std::optional<ButtonConfig> getButtonById(const std::string& id) const;

    // --- Methods to modify configuration (needed later by UI) ---
    // bool addButton(const ButtonConfig& button); 
    // bool updateButton(const std::string& id, const ButtonConfig& button);
    // bool removeButton(const std::string& id);

private:
    std::vector<ButtonConfig> m_buttons;
    std::string m_configFilePath;

    // Optional: Helper to load default config if file doesn't exist or is invalid
    void loadDefaultConfig(); 
}; 