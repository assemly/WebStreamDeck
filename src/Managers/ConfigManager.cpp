#include "ConfigManager.hpp"
#include <fstream>
#include <iostream> // For error reporting, consider a proper logger later
#include <filesystem> // For checking if file exists
#include <stdexcept> // For std::out_of_range
#include <algorithm> // For std::find_if, std::remove_if
#include <optional> // For std::optional
#include <tuple>   // For std::tuple
#include <vector>   // Ensure vector is included
#include <string>   // Ensure string is included
#include <map>      // Ensure map is included

namespace fs = std::filesystem;
using json = nlohmann::json;

// Helper to initialize the layout structure with default empty grids
void ConfigManager::initializeLayoutPages(LayoutConfig& layout) {
    layout.pages.clear();
    if (layout.page_count <= 0 || layout.rows_per_page <= 0 || layout.cols_per_page <= 0) {
        // Prevent invalid dimensions
        layout.page_count = std::max(1, layout.page_count);
        layout.rows_per_page = std::max(1, layout.rows_per_page);
        layout.cols_per_page = std::max(1, layout.cols_per_page);
        std::cerr << "Warning: Invalid layout dimensions detected during initialization. Adjusted to minimum 1x1x1." << std::endl;
    }
    for (int p = 0; p < layout.page_count; ++p) {
        layout.pages[p] = std::vector<std::vector<std::string>>(
            layout.rows_per_page,
            std::vector<std::string>(layout.cols_per_page, "") // Initialize with empty strings
        );
    }
}

ConfigManager::ConfigManager(const std::string& filename) : m_configFilePath(filename)
{
    if (!loadConfig()) {
        std::cerr << "Warning: Failed to load configuration from " << m_configFilePath 
                  << ". Loading default configuration." << std::endl;
        loadDefaultConfig(); // This now also loads default layout
        // Attempt to save immediately after loading default
        if (!saveConfig()) {
             std::cerr << "Warning: Failed to save default configuration to " << m_configFilePath << std::endl;
        }
    }
}

bool ConfigManager::loadConfig()
{
    if (!fs::exists(m_configFilePath)) {
         std::cout << "Info: Configuration file not found: " << m_configFilePath << ". Will load default." << std::endl;
         return false;
    }

    std::ifstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open configuration file: " << m_configFilePath << std::endl;
        return false;
    }

    try {
        json configJson;
        configFile >> configJson;
        configFile.close();

        // Load buttons (existing logic)
        if (configJson.contains("buttons") && configJson["buttons"].is_array()) {
            configJson.at("buttons").get_to(m_buttons);
        } else {
            std::cerr << "Warning: Config file missing 'buttons' array. Loading empty buttons." << std::endl;
            m_buttons.clear(); 
        }

        // <<< ADDED: Load layout configuration >>>
        if (configJson.contains("layout") && configJson["layout"].is_object()) {
            configJson.at("layout").get_to(m_layout);
            // Ensure loaded layout pages map is consistent with dimensions
            if (m_layout.pages.empty() || m_layout.pages.find(0) == m_layout.pages.end() || m_layout.pages.size() != static_cast<size_t>(m_layout.page_count)) {
                 std::cout << "Info: Layout pages map is missing, empty, or inconsistent with page_count. Re-initializing based on dimensions." << std::endl;
                 initializeLayoutPages(m_layout); // Re-initialize page map structure
                 // Note: This loses any potentially saved button positions if only the map was corrupted/missing.
            } else {
                // Optional: Further validation to ensure row/col dimensions match within the map
                for (const auto& pair : m_layout.pages) {
                    if (pair.second.size() != static_cast<size_t>(m_layout.rows_per_page) || (!pair.second.empty() && pair.second[0].size() != static_cast<size_t>(m_layout.cols_per_page))) {
                         std::cerr << "Warning: Mismatch detected between layout dimensions and page map structure for page " << pair.first << ". Layout might be inconsistent." << std::endl;
                         // Could re-initialize here as well, but might be too destructive.
                         break; // Stop checking after first mismatch
                    }
                }
            }
        } else {
            std::cerr << "Warning: Config file missing 'layout' object. Initializing default layout structure." << std::endl;
            m_layout = {}; // Reset to default struct values
            initializeLayoutPages(m_layout); // Initialize page map
        }

        std::cout << "Configuration loaded successfully from " << m_configFilePath << std::endl;
        return true;

    } catch (json::parse_error& e) {
        std::cerr << "Error parsing configuration file: " << m_configFilePath 
                  << "\nMessage: " << e.what() 
                  << "\nException id: " << e.id 
                  << "\nByte position of error: " << e.byte << std::endl;
        m_buttons.clear(); 
        m_layout = {}; // Reset layout on parse error
        initializeLayoutPages(m_layout); // Ensure layout is at least initialized
        return false;
    } catch (json::exception& e) {
        std::cerr << "Error processing JSON configuration in " << m_configFilePath << ": " << e.what() << std::endl;
        m_buttons.clear();
        m_layout = {};
        initializeLayoutPages(m_layout);
        return false;
    } catch (const std::exception& e) {
        std::cerr << "An unknown error occurred while loading the configuration file " << m_configFilePath << ": " << e.what() << std::endl;
        m_buttons.clear();
        m_layout = {};
        initializeLayoutPages(m_layout);
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
        json configJson;
        configJson["buttons"] = m_buttons;
        configJson["layout"] = m_layout; // <<< ADDED: Save layout object

        configFile << configJson.dump(4); // Use dump(4) for pretty printing
        configFile.close();
        
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

// --- Getters ---
const std::vector<ButtonConfig>& ConfigManager::getButtons() const
{
    return m_buttons;
}

std::optional<ButtonConfig> ConfigManager::getButtonById(const std::string& id) const
{
    auto it = std::find_if(m_buttons.begin(), m_buttons.end(),
                           [&id](const ButtonConfig& b) { return b.id == id; });
    if (it != m_buttons.end()) {
        return *it;
    }
    return std::nullopt;
}

// <<< ADDED: Implementation >>>
const LayoutConfig& ConfigManager::getLayoutConfig() const {
    return m_layout;
}

// <<< ADDED: Implementation >>>
std::string ConfigManager::getButtonIdAt(int page, int row, int col) const {
    auto pageIt = m_layout.pages.find(page);
    if (pageIt != m_layout.pages.end()) {
        const auto& grid = pageIt->second;
        if (row >= 0 && row < grid.size()) {
            const auto& rowVec = grid[row];
            if (col >= 0 && col < rowVec.size()) {
                return rowVec[col];
            }
        }
    }
    return ""; // Return empty string if position is invalid or page not found
}

// --- Default Config ---
void ConfigManager::loadDefaultConfig()
{
    std::cout << "Loading default configuration (buttons and layout)." << std::endl;
    // Default buttons (assuming the list from previous interaction)
    m_buttons = {
        {"btn_notepad", "Notepad1", "launch_app", "notepad.exe", "assets/icons/puppy.gif"},
        {"btn_calc", "Calculator", "launch_app", "calc.exe", "assets/icons/calculator.204x256.png"},
        {"btn_bilibili", "b站", "open_url", "https://space.bilibili.com/5324474?spm_id_from=333.1007.0.0", "assets/icons/bilibili_round-384x384.png"}, // User updated
        {"BTN_ADD", "音量增大", "media_volume_up", "", "assets/icons/volume-up.256x232.png"}, // Updated path
        {"btn", "音量减少", "media_mute", "", "assets/icons/volume-down.256x232.png"}, // Updated path
        {"bt_bilibil_", "b站快进", "hotkey", "[", "assets/icons/puppy.gif"}, // User updated
        {"btn_capture", "截图", "hotkey", "CTRL+A", "assets/icons/puppy.gif"}, // User updated
        {"btn_wechat", "微信", "launch_app", "WeChat.exe", "assets/icons/wechat.256x256.png"},
        {"btn_qq", "QQ", "launch_app", "QQ.exe", "assets/icons/qq.216x256.png"}, // Updated path
        {"btn_dingtalk", "钉钉", "launch_app", "DingTalk.exe", "assets/icons/dingding.203x256.png"}, // User updated
        {"btn_netease_music", "网易云音乐", "launch_app", "cloudmusic.exe", "assets/icons/netease-cloud-music.255x256.png"}, // Updated path
        {"btn_copy", "复制", "hotkey", "CTRL+C", "assets/icons/copy.256x256.png"}, // Updated path
        {"btn_paste", "粘贴", "hotkey", "CTRL+V", "assets/icons/content-paste.210x256.png"}, // Updated path
        {"btn_lock_screen", "锁定屏幕", "hotkey", "WIN+L", "assets/icons/gnome-lockscreen.256x253.png"}, // Updated path
        {"btn_task_manager", "任务管理器", "hotkey", "CTRL+SHIFT+ESC", "assets/icons/task_manager.png"},
        {"btn_baidu", "百度", "open_url", "https://www.baidu.com", "assets/icons/baidu.234x256.png"}, // Updated path
        {"btn_taobao", "淘宝", "open_url", "https://www.taobao.com", "assets/icons/taobao-circle.256x256.png"}, // Updated path
        {"btn_jd", "京东", "open_url", "https://www.jd.com", "assets/icons/jd-gui.256x256.png"}, // Updated path
        {"btn_weibo", "微博", "open_url", "https://weibo.com", "assets/icons/weibo.256x208.png"} // Updated path
    };

    // <<< ADDED: Default layout >>>
    m_layout = {}; // Reset to default struct values first
    m_layout.page_count = 1;
    m_layout.rows_per_page = 3;
    m_layout.cols_per_page = 5;
    initializeLayoutPages(m_layout); // Initialize the pages map structure

    // Place some default buttons on the first page (page 0)
    // Check bounds before assigning
    if (m_layout.rows_per_page > 0 && m_layout.cols_per_page > 0) m_layout.pages[0][0][0] = "btn_notepad";
    if (m_layout.rows_per_page > 0 && m_layout.cols_per_page > 1) m_layout.pages[0][0][1] = "btn_calc";
    if (m_layout.rows_per_page > 0 && m_layout.cols_per_page > 2) m_layout.pages[0][0][2] = "btn_bilibili";
    if (m_layout.rows_per_page > 0 && m_layout.cols_per_page > 3) m_layout.pages[0][0][3] = "btn_wechat";
    if (m_layout.rows_per_page > 0 && m_layout.cols_per_page > 4) m_layout.pages[0][0][4] = "btn_qq";
    if (m_layout.rows_per_page > 1 && m_layout.cols_per_page > 0) m_layout.pages[0][1][0] = "btn_copy";
    if (m_layout.rows_per_page > 1 && m_layout.cols_per_page > 1) m_layout.pages[0][1][1] = "btn_paste";
    if (m_layout.rows_per_page > 1 && m_layout.cols_per_page > 2) m_layout.pages[0][1][2] = "btn_lock_screen";
    if (m_layout.rows_per_page > 1 && m_layout.cols_per_page > 3) m_layout.pages[0][1][3] = "BTN_ADD"; // Vol Up
    if (m_layout.rows_per_page > 1 && m_layout.cols_per_page > 4) m_layout.pages[0][1][4] = "btn";     // Vol Down (using ID 'btn')
    if (m_layout.rows_per_page > 2 && m_layout.cols_per_page > 2) m_layout.pages[0][2][2] = "btn_task_manager";

    // Ensure page_count is set correctly (at least 1 if page 0 was populated)
    if (m_layout.pages.empty()) {
        m_layout.page_count = 0;
    } else {
        // For now, just set to 1 if page 0 exists. 
        // A more robust approach would find the max page index used.
        m_layout.page_count = 1; 
    }
}

// --- Modifiers ---
bool ConfigManager::addButton(const ButtonConfig& button)
{
    if (button.id.empty() || button.name.empty()) {
        std::cerr << "Error: Cannot add button with empty ID or Name." << std::endl;
        return false;
    }
    // Check for duplicate ID
    if (getButtonById(button.id).has_value()) {
             std::cerr << "Error: Button with ID '" << button.id << "' already exists." << std::endl;
            return false;
    }
    m_buttons.push_back(button);
    // NOTE: Adding a button doesn't automatically place it in the layout.
    // UI needs to call setButtonPosition separately.
    std::cout << "Button '" << button.id << "' added." << std::endl;
    return saveConfig(); // Save after adding
}

bool ConfigManager::updateButton(const std::string& id, const ButtonConfig& updatedButton)
{
    if (updatedButton.name.empty()) {
        std::cerr << "Error: Updated button name cannot be empty." << std::endl;
        return false;
    }
    if (updatedButton.id != id) {
         std::cerr << "Error: Mismatched ID during update attempt. ID cannot be changed." << std::endl;
         return false;
    }
    for (auto& button : m_buttons) {
        if (button.id == id) {
            button.name = updatedButton.name;
            button.action_type = updatedButton.action_type;
            button.action_param = updatedButton.action_param;
            button.icon_path = updatedButton.icon_path;
            std::cout << "Button '" << id << "' updated." << std::endl;
            return saveConfig(); // Save after updating
        }
    }
    std::cerr << "Error: Button with ID '" << id << "' not found for update." << std::endl;
    return false;
}

bool ConfigManager::removeButton(const std::string& id)
{
    if (id.empty()) return false;

    auto it = std::remove_if(m_buttons.begin(), m_buttons.end(), 
                             [&id](const ButtonConfig& b){ return b.id == id; });

    if (it != m_buttons.end()) {
        m_buttons.erase(it, m_buttons.end());
        std::cout << "Button '" << id << "' removed from button list." << std::endl;

        // <<< ADDED: Remove button from layout when deleted >>>
        int cleared_count = 0;
        for (auto& page_pair : m_layout.pages) {
            for (auto& row : page_pair.second) {
                for (auto& cell_id : row) {
                    if (cell_id == id) {
                        cell_id = ""; // Clear the slot
                        cleared_count++;
                    }
                }
            }
        }
         if (cleared_count > 0) {
             std::cout << "Button '" << id << "' cleared from " << cleared_count << " layout position(s)." << std::endl;
         }
        return saveConfig(); // Save after removing
    }
    std::cerr << "Button with ID '" << id << "' not found for removal." << std::endl;
    return false;
}

// <<< ADDED: Implementation for layout modifiers >>>
bool ConfigManager::setButtonPosition(const std::string& buttonId, int page, int row, int col) {
     // Check if the button ID exists (allow setting empty string "" to clear a position)
     if (!buttonId.empty() && !getButtonById(buttonId).has_value()) {
         std::cerr << "Error setting position: Button with ID '" << buttonId << "' does not exist." << std::endl;
         return false;
     }

    try {
        // Check page bounds (using page_count)
        if (page < 0 || page >= m_layout.page_count) {
             std::cerr << "Error setting position: Page index " << page << " is out of bounds (0-" << m_layout.page_count - 1 << ")." << std::endl;
             return false;
        }
        // Ensure page exists in map (it should if initialized correctly)
        auto page_it = m_layout.pages.find(page);
        if (page_it == m_layout.pages.end()) {
             std::cerr << "Error: Page " << page << " unexpectedly not found in layout map. Re-initializing might be needed." << std::endl;
             return false;
        }
        // Check row bounds (using rows_per_page)
        if (row < 0 || row >= m_layout.rows_per_page) {
            std::cerr << "Error setting position: Row index " << row << " is out of bounds (0-" << m_layout.rows_per_page - 1 << ")." << std::endl;
            return false;
        }
        // Check col bounds (using cols_per_page)
        if (col < 0 || col >= m_layout.cols_per_page) {
             std::cerr << "Error setting position: Column index " << col << " is out of bounds (0-" << m_layout.cols_per_page - 1 << ")." << std::endl;
            return false;
        }
         // Ensure the target row/column indices are within the actual page map dimensions (using .at() implicitly checks this later)
         if (static_cast<size_t>(row) >= page_it->second.size() || static_cast<size_t>(col) >= page_it->second[row].size()){
             std::cerr << "Error setting position: Target position (" << row << "," << col << ") exceeds actual dimensions of page " << page << " map." << std::endl;
             return false;
         }


        // --- Check if button is already placed elsewhere and clear that spot ---
        // We only need to clear if we are placing a *non-empty* button ID
        if (!buttonId.empty()) {
            std::string currentIdAtTarget = page_it->second.at(row).at(col); // Use .at() for safety
            if (currentIdAtTarget == buttonId) {
                std::cout << "Button '" << buttonId << "' is already at the target position." << std::endl;
                return true; // Already in the correct spot, no change, no save needed.
            }

            // If buttonId is not empty, iterate and clear its previous position(s)
            bool cleared_old = false;
            for (auto& current_page_pair : m_layout.pages) {
                 // Check if the current page has the correct number of rows
                if (current_page_pair.second.size() != static_cast<size_t>(m_layout.rows_per_page)) continue;

                for (size_t r = 0; r < current_page_pair.second.size(); ++r) {
                    // Check if the current row has the correct number of columns
                    if (current_page_pair.second[r].size() != static_cast<size_t>(m_layout.cols_per_page)) continue;

                    for (size_t c = 0; c < current_page_pair.second[r].size(); ++c) {
                        // Don't clear the target spot itself if we are iterating over it
                        if (static_cast<int>(r) == row && static_cast<int>(c) == col && current_page_pair.first == page) {
                            continue;
                        }
                        if (current_page_pair.second[r][c] == buttonId) {
                            current_page_pair.second[r][c] = ""; // Clear old spot
                             std::cout << "Cleared old position of button '" << buttonId << "' at Page " << current_page_pair.first << ", Row " << r << ", Col " << c << std::endl;
                             cleared_old = true;
                             // Assume button ID is unique in layout, so break loops? Safer not to assume for now.
                        }
                    }
                }
            }
        }
        // --- End of clear old spot ---

        // Set the button ID at the new position using .at() for safety
        page_it->second.at(row).at(col) = buttonId;

        if (buttonId.empty()) {
             std::cout << "Cleared button at Page " << page << ", Row " << row << ", Col " << col << std::endl;
        } else {
             std::cout << "Set button '" << buttonId << "' at Page " << page << ", Row " << row << ", Col " << col << std::endl;
        }
        return saveConfig(); // Save after changing layout

    } catch (const std::out_of_range& oor) { // Catch potential out_of_range from map/vector access using .at()
         std::cerr << "Error setting button position (out_of_range): " << oor.what() << std::endl;
         return false;
    } catch (const std::exception& e) { // Catch other potential errors
        std::cerr << "Unknown error setting button position: " << e.what() << std::endl;
        return false;
    }
}

// <<< ADDED: Implementation >>>
bool ConfigManager::clearButtonPosition(int page, int row, int col) {
    auto pageIt = m_layout.pages.find(page);
    if (pageIt != m_layout.pages.end()) {
        auto& grid = pageIt->second;
        if (row >= 0 && row < grid.size()) {
            auto& rowVec = grid[row];
            if (col >= 0 && col < rowVec.size()) {
                if (!rowVec[col].empty()) { // Only save if something was actually cleared
                    rowVec[col] = "";
                    return saveConfig();
                }
                return true; // Position was already empty, technically success
            }
        }
    }
    std::cerr << "Error: Cannot clear button at invalid position (Page: " << page << ", Row: " << row << ", Col: " << col << ")" << std::endl;
    return false;
}

// Finds the old position of a button and clears it.
// Returns true if the button was found and cleared (regardless of save success), false otherwise.
// Internal helper, does not save config.
bool ConfigManager::findAndClearOldPosition(const std::string& buttonId) {
    for (auto& page_pair : m_layout.pages) {
        for (auto& row_vec : page_pair.second) {
            for (auto& cell_id : row_vec) {
                if (cell_id == buttonId) {
                    cell_id = ""; // Clear the old position
                    return true; // Found and cleared
                }
            }
        }
    }
    return false; // Button not found in layout
}

// <<< ADDED: Implementation for findButtonPosition >>>
std::optional<std::tuple<int, int, int>> ConfigManager::findButtonPosition(const std::string& buttonId) const {
    if (buttonId.empty()) {
        return std::nullopt;
    }
    for (const auto& page_pair : m_layout.pages) {
        int page = page_pair.first;
        const auto& grid = page_pair.second;
        for (int r = 0; r < grid.size(); ++r) {
             // Ensure row size matches configured cols for safety, although layout should be consistent
            if (r < m_layout.rows_per_page && grid[r].size() == m_layout.cols_per_page) { 
                for (int c = 0; c < grid[r].size(); ++c) {
                    if (grid[r][c] == buttonId) {
                        return std::make_tuple(page, r, c);
                    }
                }
            }
        }
    }
    return std::nullopt; // Not found
}

// <<< ADDED: Implementation for swapButtons >>>
bool ConfigManager::swapButtons(const std::string& buttonId1, const std::string& buttonId2) {
    if (buttonId1.empty() || buttonId2.empty() || buttonId1 == buttonId2) {
        std::cerr << "Error: Invalid button IDs provided for swap." << std::endl;
        return false; // Cannot swap empty or identical IDs
    }

    auto pos1Opt = findButtonPosition(buttonId1);
    auto pos2Opt = findButtonPosition(buttonId2);

    if (!pos1Opt) {
        std::cerr << "Error: Cannot swap, button ID not found in layout: " << buttonId1 << std::endl;
        return false;
    }
    if (!pos2Opt) {
        std::cerr << "Error: Cannot swap, button ID not found in layout: " << buttonId2 << std::endl;
        return false;
    }

    auto [page1, row1, col1] = *pos1Opt;
    auto [page2, row2, col2] = *pos2Opt;

    // Directly swap the IDs in the layout map
    // We need write access, so use operator[] which might create pages if they don't exist (shouldn't happen here)
    // It's safer to check page existence first but findButtonPosition already implies they exist.
    try {
        std::cout << "Swapping '" << buttonId1 << "' at [" << page1 << "," << row1 << "," << col1 
                  << "] with '" << buttonId2 << "' at [" << page2 << "," << row2 << "," << col2 << "]" << std::endl;
        m_layout.pages.at(page1).at(row1).at(col1) = buttonId2;
        m_layout.pages.at(page2).at(row2).at(col2) = buttonId1;
    } catch (const std::out_of_range& e) {
        // This should ideally not happen if findButtonPosition worked correctly
        std::cerr << "Error: Out of range access during swap operation. Layout data might be inconsistent. " << e.what() << std::endl;
        return false;
    }

    // Save the changes
    if (!saveConfig()) {
        std::cerr << "Error: Failed to save configuration after swapping buttons." << std::endl;
        // Attempt to revert the swap in memory? Or just report error.
        // Reverting:
        try {
             m_layout.pages.at(page1).at(row1).at(col1) = buttonId1;
             m_layout.pages.at(page2).at(row2).at(col2) = buttonId2;
             std::cerr << "Attempted to revert swap in memory due to save failure." << std::endl;
        } catch (...) { /* Ignore errors during revert attempt */ }
        return false;
    }

    return true;
}

// <<< MODIFIED: Implementation for setLayoutDimensions with relocation >>>
bool ConfigManager::setLayoutDimensions(int newPageCount, int newRows, int newCols) {
    if (newPageCount < 1 || newRows < 1 || newCols < 1) {
        std::cerr << "[ConfigManager] Error: Invalid layout dimensions provided (must be >= 1)." << std::endl;
        return false;
    }

    std::cout << "[ConfigManager] Starting layout dimension update to [P:" << newPageCount 
              << ", R:" << newRows << ", C:" << newCols << "]" << std::endl;

    // 1. Backup & Collect old placements
    std::vector<std::tuple<int, int, int, std::string>> oldPlacements;
    std::map<std::string, bool> placedButtons; // Keep track of buttons placed
    for (const auto& pagePair : m_layout.pages) {
        int page = pagePair.first;
        const auto& grid = pagePair.second;
        for (int r = 0; r < grid.size(); ++r) {
            const auto& rowVec = grid[r];
            for (int c = 0; c < rowVec.size(); ++c) {
                if (!rowVec[c].empty()) {
                    oldPlacements.emplace_back(page, r, c, rowVec[c]);
                }
            }
        }
    }
    std::cout << "[ConfigManager] Collected " << oldPlacements.size() << " existing button placements." << std::endl;

    // 2. Create new empty layout structure
    std::map<int, std::vector<std::vector<std::string>>> newPages;
    for (int p = 0; p < newPageCount; ++p) {
        newPages[p] = std::vector<std::vector<std::string>>(newRows, std::vector<std::string>(newCols, ""));
    }

    // 3. Place buttons that remain within bounds
    std::vector<std::string> buttonsToRelocate;
    for (const auto& placement : oldPlacements) {
        int oldPage, oldRow, oldCol;
        std::string buttonId;
        std::tie(oldPage, oldRow, oldCol, buttonId) = placement;

        // Check if the old position is still valid in the new layout
        if (oldPage < newPageCount && oldRow < newRows && oldCol < newCols) {
            // Check if target slot is empty (it should be, but safety check)
            if (newPages[oldPage][oldRow][oldCol].empty()) {
                newPages[oldPage][oldRow][oldCol] = buttonId;
                placedButtons[buttonId] = true;
                // std::cout << "  - Kept button '" << buttonId << "' at original position [" << oldPage << "," << oldRow << "," << oldCol << "]" << std::endl;
            } else {
                 // This case means a button was already placed here, likely due to duplicate IDs in old layout?
                 // Place the current one elsewhere if possible
                 std::cerr << "[ConfigManager] Warning: Target slot [" << oldPage << "," << oldRow << "," << oldCol << "] was already occupied when trying to keep button '" << buttonId << "'. Marking for relocation." << std::endl;
                 buttonsToRelocate.push_back(buttonId);
            }
        } else {
            // Mark button for relocation if its original position is out of bounds
            buttonsToRelocate.push_back(buttonId);
            // std::cout << "  - Marked button '" << buttonId << "' from old position [" << oldPage << "," << oldRow << "," << oldCol << "] for relocation." << std::endl;
        }
    }
     std::cout << "[ConfigManager] Buttons kept at original relative positions: " << placedButtons.size() 
               << ". Buttons needing relocation: " << buttonsToRelocate.size() << std::endl;


    // 4. Find available empty slots in the new layout
    std::vector<std::tuple<int, int, int>> availableSlots;
    for (int p = 0; p < newPageCount; ++p) {
        for (int r = 0; r < newRows; ++r) {
            for (int c = 0; c < newCols; ++c) {
                if (newPages[p][r][c].empty()) {
                    availableSlots.emplace_back(p, r, c);
                }
            }
        }
    }
    std::cout << "[ConfigManager] Found " << availableSlots.size() << " available empty slots in the new layout." << std::endl;

    // 5. Relocate displaced buttons
    int relocatedCount = 0;
    int discardedCount = 0;
    for (const auto& buttonId : buttonsToRelocate) {
        // Check if this button was already placed (e.g., kept at original position)
        if (placedButtons.count(buttonId)) {
             // std::cout << "  - Button '" << buttonId << "' was already kept, skipping relocation attempt." << std::endl;
             continue; 
        }

        if (!availableSlots.empty()) {
            // Get the next available slot
            int nextPage, nextRow, nextCol;
            std::tie(nextPage, nextRow, nextCol) = availableSlots.front();
            availableSlots.erase(availableSlots.begin()); // Remove the used slot

            // Place the button
            newPages[nextPage][nextRow][nextCol] = buttonId;
            placedButtons[buttonId] = true; // Mark as placed
            relocatedCount++;
            std::cout << "  - Relocated button '" << buttonId << "' to new position [" << nextPage << "," << nextRow << "," << nextCol << "]" << std::endl;
        } else {
            // No more empty slots, discard the button from layout
            discardedCount++;
            std::cout << "  - Discarded button '" << buttonId << "' from layout (no available slots)." << std::endl;
            // Note: The button itself still exists in m_buttons, just not in the layout
        }
    }
    std::cout << "[ConfigManager] Relocated " << relocatedCount << " buttons. Discarded " << discardedCount << " buttons from layout." << std::endl;

    // 6. Update internal layout state and save
    m_layout.pages = std::move(newPages); // Replace old map with the newly constructed one
    m_layout.page_count = newPageCount;
    m_layout.rows_per_page = newRows;
    m_layout.cols_per_page = newCols;

    if (!saveConfig()) {
        std::cerr << "[ConfigManager] Error: Failed to save config after updating layout dimensions! State may be inconsistent." << std::endl;
        // Difficult to revert reliably here, rely on user to potentially fix manually or reload previous config
        return false;
    }

    std::cout << "[ConfigManager] Layout dimensions updated, buttons relocated/discarded, and configuration saved." << std::endl;
    return true;
}

// <<< ADDED: Implementation for loading config from a specific file >>>
bool ConfigManager::loadConfigFromFile(const std::string& filePath) {
    std::cout << "[ConfigManager] Attempting to load preset configuration from: " << filePath << std::endl;
    if (!fs::exists(filePath)) {
         std::cerr << "[ConfigManager] Error: Preset file not found: " << filePath << std::endl;
         return false;
    }

    std::ifstream presetFile(filePath);
    if (!presetFile.is_open()) {
        std::cerr << "[ConfigManager] Error: Could not open preset file: " << filePath << std::endl;
        return false;
    }

    try {
        json configJson;
        presetFile >> configJson;
        presetFile.close();

        // Temporary storage for loaded data
        std::vector<ButtonConfig> tempButtons;
        LayoutConfig tempLayout;

        // Load buttons
        if (configJson.contains("buttons") && configJson["buttons"].is_array()) {
            configJson.at("buttons").get_to(tempButtons);
        } else {
            std::cerr << "[ConfigManager] Warning: Preset file missing 'buttons' array. Loading empty buttons." << std::endl;
            tempButtons.clear(); 
        }

        // Load layout
        if (configJson.contains("layout") && configJson["layout"].is_object()) {
            configJson.at("layout").get_to(tempLayout);
            // Basic validation after loading preset layout
            if (tempLayout.page_count <= 0 || tempLayout.rows_per_page <= 0 || tempLayout.cols_per_page <= 0) {
                 std::cerr << "[ConfigManager] Warning: Preset file contains invalid layout dimensions. Resetting to default." << std::endl;
                 tempLayout = {}; // Reset to default struct values
                 initializeLayoutPages(tempLayout);
            } else {
                 // Ensure map consistency (similar to loadConfig)
                 if (tempLayout.pages.empty() || tempLayout.pages.size() != static_cast<size_t>(tempLayout.page_count)) {
                     std::cout << "[ConfigManager] Info: Preset layout pages map inconsistent. Re-initializing." << std::endl;
                     initializeLayoutPages(tempLayout);
                 } else {
                      // Optional: Further validation of row/col dimensions within map
                 }
            }
        } else {
            std::cerr << "[ConfigManager] Warning: Preset file missing 'layout' object. Initializing default layout structure." << std::endl;
            tempLayout = {};
            initializeLayoutPages(tempLayout);
        }

        // --- Apply loaded data to ConfigManager --- 
        m_buttons = std::move(tempButtons);
        m_layout = std::move(tempLayout);

        // <<< IMPORTANT: Overwrite the main config path and save >>>
        // We assume loading a preset makes it the new main config.
        // If you want presets to be temporary, skip saveConfig().
        std::cout << "[ConfigManager] Preset loaded successfully. Saving as current configuration..." << std::endl;
        if (!saveConfig()) {
            std::cerr << "[ConfigManager] Error: Failed to save the loaded preset to the main config file: " << m_configFilePath << std::endl;
            // State is already updated in memory, but save failed.
            return false; // Indicate save failure, though load succeeded.
        }

        std::cout << "[ConfigManager] Preset loaded and saved as current config." << std::endl;
        return true; // Load and save successful

    } catch (json::parse_error& e) {
        std::cerr << "[ConfigManager] Error parsing preset file: " << filePath << "\nMessage: " << e.what() << std::endl;
        return false;
    } catch (json::exception& e) {
        std::cerr << "[ConfigManager] Error processing JSON in preset file " << filePath << ": " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] An unknown error occurred while loading the preset file " << filePath << ": " << e.what() << std::endl;
        return false;
    }
}

// <<< ADDED: Implementation for saving config to a new preset file >>>
bool ConfigManager::saveConfigToPreset(const std::string& presetName) {
    if (presetName.empty()) {
        std::cerr << "[ConfigManager] Error: Preset name cannot be empty." << std::endl;
        return false;
    }

    // Basic validation for preset name (prevent directory traversal, etc.)
    // Replace potentially problematic characters? For simplicity, just check for slashes/backslashes.
    if (presetName.find('/') != std::string::npos || presetName.find('\\') != std::string::npos) {
        std::cerr << "[ConfigManager] Error: Preset name cannot contain path separators (/ or \\)." << std::endl;
        return false;
    }

    // Ensure the presets directory exists
    const std::string presetsDir = "assets/presetconfig";
    try {
        if (!fs::exists(presetsDir)) {
            std::cout << "[ConfigManager] Presets directory not found, creating: " << presetsDir << std::endl;
            if (!fs::create_directories(presetsDir)) {
                std::cerr << "[ConfigManager] Error: Failed to create presets directory: " << presetsDir << std::endl;
                return false;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ConfigManager] Filesystem error checking/creating presets directory: " << e.what() << std::endl;
        return false;
    }

    // Construct the full path for the new preset file
    std::string presetFilePath = presetsDir + "/" + presetName + ".json";
    std::cout << "[ConfigManager] Attempting to save current configuration as preset: " << presetFilePath << std::endl;

    // Open the file for writing (this will create or overwrite)
    std::ofstream presetFile(presetFilePath);
    if (!presetFile.is_open()) {
        std::cerr << "[ConfigManager] Error: Could not open preset file for writing: " << presetFilePath << std::endl;
        return false;
    }

    // Serialize current m_buttons and m_layout to JSON
    try {
        json configJson;
        configJson["buttons"] = m_buttons;
        configJson["layout"] = m_layout;

        presetFile << configJson.dump(4); // Pretty print
        presetFile.close();

        std::cout << "[ConfigManager] Current configuration saved successfully as preset: " << presetFilePath << std::endl;
        return true;

    } catch (json::exception& e) {
        std::cerr << "[ConfigManager] Error creating JSON for preset file " << presetFilePath << ": " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
         std::cerr << "[ConfigManager] An unknown error occurred while saving the preset file " << presetFilePath << ": " << e.what() << std::endl;
        return false;
    }
} 