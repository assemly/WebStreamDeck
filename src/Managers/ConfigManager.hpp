#pragma once

#include <string>
#include <vector>
#include <optional> // Include for optional return type
#include <map> // Added for layout pages map
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

// Define structure for layout configuration
struct LayoutConfig {
    int page_count = 1;
    int rows_per_page = 3;
    int cols_per_page = 5;
    // Map from page index to a 2D vector representing the grid of button IDs
    std::map<int, std::vector<std::vector<std::string>>> pages;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LayoutConfig, page_count, rows_per_page, cols_per_page, pages);
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

    // Get layout configuration
    const LayoutConfig& getLayoutConfig() const;

    // Get button ID at a specific position in the layout
    std::string getButtonIdAt(int page, int row, int col) const;

    // --- Methods to modify configuration (needed later by UI) ---
    bool addButton(const ButtonConfig& button);
    bool updateButton(const std::string& id, const ButtonConfig& button);
    bool removeButton(const std::string& id);

    // Modifiers for layout
    bool setButtonPosition(const std::string& buttonId, int page, int row, int col);
    bool clearButtonPosition(int page, int row, int col);

private:
    std::vector<ButtonConfig> m_buttons;
    LayoutConfig m_layout; // Layout configuration member
    std::string m_configFilePath;

    // Optional: Helper to load default config if file doesn't exist or is invalid
    void loadDefaultConfig();

    // Helper to initialize layout pages based on dimensions
    static void initializeLayoutPages(LayoutConfig& layout);
}; 