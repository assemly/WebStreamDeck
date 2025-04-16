#include "UIStatusLogWindow.hpp"
#include <iostream> // For std::cout, std::cerr
#include <fstream>  // For file I/O
#include <string>   // For std::string manipulation
#include <vector>   // Used indirectly by windows.h potentially
#include <locale>   // For string conversions
#include <codecvt>  // For string conversions

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // Include Windows header for Registry functions
#endif

// Helper function to trim whitespace from a string (needed for parsing)
std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

#ifdef _WIN32
// Helper function to convert UTF-8 std::string to std::wstring
std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Helper function to convert std::wstring to UTF-8 std::string
std::string wstring_to_utf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// --- Windows Registry Startup Functions ---
const wchar_t* StartupRegistryKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

// Function to add the application to Windows startup
bool RegisterForStartup(const std::wstring& appName, const std::wstring& appPath) {
    HKEY hKey = NULL;
    // Quote the path in case it contains spaces
    std::wstring quotedPath = L"\"" + appPath + L"\"";

    // Open the Run key
    LONG lResult = RegOpenKeyExW(HKEY_CURRENT_USER, StartupRegistryKey, 0, KEY_WRITE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Error: Could not open registry key HKEY_CURRENT_USER\\" << wstring_to_utf8(StartupRegistryKey) << " for writing. Error code: " << lResult << std::endl;
        return false;
    }

    // Set the value (application name = path to executable)
    lResult = RegSetValueExW(hKey, appName.c_str(), 0, REG_SZ,
                             (const BYTE*)quotedPath.c_str(),
                             (quotedPath.length() + 1) * sizeof(wchar_t)); // +1 for null terminator

    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Error: Could not set registry value '" << wstring_to_utf8(appName) << "'. Error code: " << lResult << std::endl;
        return false;
    }

    std::cout << "Successfully registered '" << wstring_to_utf8(appName) << "' for startup." << std::endl;
    return true;
}

// Function to remove the application from Windows startup
bool UnregisterFromStartup(const std::wstring& appName) {
    HKEY hKey = NULL;

    // Open the Run key
    LONG lResult = RegOpenKeyExW(HKEY_CURRENT_USER, StartupRegistryKey, 0, KEY_WRITE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Error: Could not open registry key HKEY_CURRENT_USER\\" << wstring_to_utf8(StartupRegistryKey) << " for writing. Error code: " << lResult << std::endl;
        return false;
    }

    // Delete the value
    lResult = RegDeleteValueW(hKey, appName.c_str());
    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) { // Ignore if value already doesn't exist
        std::cerr << "Error: Could not delete registry value '" << wstring_to_utf8(appName) << "'. Error code: " << lResult << std::endl;
        return false;
    }

     if (lResult == ERROR_FILE_NOT_FOUND) {
         std::cout << "Startup registry value '" << wstring_to_utf8(appName) << "' not found (already unregistered?)." << std::endl;
     } else {
         std::cout << "Successfully unregistered '" << wstring_to_utf8(appName) << "' from startup." << std::endl;
     }
    return true;
}

#endif // _WIN32

UIStatusLogWindow::UIStatusLogWindow(TranslationManager& translationManager)
    : m_translator(translationManager) { // m_startOnBoot is initialized by LoadConfig now
    LoadConfig(); // Load settings on construction

    // Initialize language index based on current translator language
    const auto& availableLangs = m_translator.getAvailableLanguages();
    const std::string& currentLang = m_translator.getCurrentLanguage();
    m_currentLangIndex = -1;
    for (size_t i = 0; i < availableLangs.size(); ++i) {
        if (availableLangs[i] == currentLang) {
            m_currentLangIndex = static_cast<int>(i);
            break;
        }
    }
     if (m_currentLangIndex == -1 && !availableLangs.empty()) {
         m_currentLangIndex = 0; // Default to first language if current not found
     }
}


void UIStatusLogWindow::Draw(bool isServerRunning, int serverPort, const std::string& serverIP, std::function<void()> refreshIpCallback) {
    ImGui::Begin(m_translator.get("status_log_window_title").c_str());

    // --- Server Status Display ---
    ImGui::Text("%s %s", m_translator.get("server_status_label").c_str(),
              isServerRunning ? m_translator.get("server_status_running").c_str()
                                : m_translator.get("server_status_stopped").c_str());

    // Check if IP is valid before displaying addresses
    bool isIpValid = (serverIP.find("Error") == std::string::npos &&
                      serverIP.find("Fetching") == std::string::npos &&
                      serverIP.find("No suitable") == std::string::npos &&
                      !serverIP.empty());

    if (isServerRunning && serverPort > 0 && isIpValid) {
         ImGui::Text("%s http://%s:%d", m_translator.get("web_ui_address_label").c_str(), serverIP.c_str(), serverPort);
         ImGui::Text("%s ws://%s:%d", m_translator.get("websocket_address_label").c_str(), serverIP.c_str(), serverPort);
    } else {
        ImGui::Text("%s %s", m_translator.get("server_address_label").c_str(),
                  m_translator.get("server_address_error").c_str());
    }

    // Refresh IP Button - Use the callback provided by UIManager
    if (ImGui::Button(m_translator.get("refresh_ip_button").c_str())) {
        if (refreshIpCallback) {
            refreshIpCallback();
        }
    }
    ImGui::Separator();

    // --- Logs Section ---
    ImGui::TextUnformatted(m_translator.get("logs_header").c_str());
    // TODO: Implement actual logging display here (needs access to a log buffer/system)
    ImGui::TextWrapped("Log display area placeholder..."); // Placeholder
    ImGui::Separator();

    // --- Language Selection ---
    const auto& availableLangs = m_translator.getAvailableLanguages();
    std::vector<const char*> langItems;
    langItems.reserve(availableLangs.size()); // Pre-allocate memory
    for (const auto& lang : availableLangs) {
        langItems.push_back(lang.c_str());
    }

    // Ensure index is valid before using it
    if (m_currentLangIndex < 0 || m_currentLangIndex >= static_cast<int>(availableLangs.size())) {
       if (!availableLangs.empty()) m_currentLangIndex = 0; // Default to first if out of bounds
       else m_currentLangIndex = -1; // No languages available
    }


    ImGui::TextUnformatted("Language / 语言:");
    ImGui::SameLine();
    ImGui::PushItemWidth(150); // Give combo box a reasonable width
    if (ImGui::Combo("##LangCombo", &m_currentLangIndex, langItems.data(), langItems.size())) {
        if (m_currentLangIndex >= 0 && m_currentLangIndex < static_cast<int>(availableLangs.size())) {
            const std::string& selectedLang = availableLangs[m_currentLangIndex];
            if (m_translator.setLanguage(selectedLang)) {
                std::cout << "Language changed to: " << selectedLang << std::endl;
                // Potentially trigger font rebuild if necessary (though often handled elsewhere)
            } else {
                 std::cerr << "Failed to set language to: " << selectedLang << std::endl;
                 // Revert index? Or maybe translator handles this internally?
                 // For now, just log the error. The index might be out of sync if setLanguage fails.
                 // Re-fetch the index after attempt might be safer:
                 const std::string& actualLang = m_translator.getCurrentLanguage();
                 for(size_t i = 0; i < availableLangs.size(); ++i) {
                     if (availableLangs[i] == actualLang) {
                         m_currentLangIndex = static_cast<int>(i);
                         break;
                     }
                 }
            }
        }
    }
    ImGui::PopItemWidth();

    ImGui::Separator(); // Add a separator before the new setting

    // --- Settings Section ---
    ImGui::TextUnformatted(m_translator.get("settings_header").c_str());

    if (ImGui::Checkbox(m_translator.get("setting_start_on_boot").c_str(), &m_startOnBoot)) {
        std::cout << "Start on boot setting changed to: " << (m_startOnBoot ? "true" : "false") << std::endl;

        SaveConfig(); // Save settings when changed

        // --- Actual System Integration --- 
#ifdef _WIN32
        wchar_t pathBuffer[MAX_PATH];
        std::wstring executablePath = L""; // Initialize empty
        DWORD pathLen = GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);

        if (pathLen == 0) {
             std::cerr << "Error: GetModuleFileNameW failed. Error code: " << GetLastError() << std::endl;
        } else if (pathLen == MAX_PATH) {
            std::cerr << "Error: Executable path exceeds MAX_PATH length." << std::endl;
        } else {
            executablePath = pathBuffer;
        }

        // Only proceed if we successfully got the path
        if (!executablePath.empty()) {
             std::wstring appName = L"WebStreamDeck"; // Name for the registry entry

             bool success = false;
             if (m_startOnBoot) {
                 success = RegisterForStartup(appName, executablePath);
             } else {
                 success = UnregisterFromStartup(appName);
             }

             if (!success) {
                 std::cerr << "Warning: Failed to update Windows startup settings. Please check permissions or run as administrator if needed." << std::endl;
                 // Optional: Revert the checkbox state if registry update failed?
                 // m_startOnBoot = !m_startOnBoot;
                 // SaveConfig(); // Save the reverted state
             }
        } else {
            std::cerr << "Error: Cannot update startup settings because executable path could not be determined." << std::endl;
             // Revert the checkbox state as we couldn't perform the action
             m_startOnBoot = !m_startOnBoot;
             SaveConfig(); // Save the reverted state
        }
#else
        std::cout << "Warning: Start on boot configuration is only implemented for Windows currently." << std::endl;
#endif
    }

    if (ImGui::Checkbox(m_translator.get("setting_start_minimized").c_str(), &m_startMinimized)) {
        std::cout << "Start Minimized setting changed to: " << (m_startMinimized ? "true" : "false") << std::endl;
        SaveConfig();
    }

    ImGui::End();
}

// --- Configuration Loading/Saving Implementation ---

void UIStatusLogWindow::LoadConfig() {
    m_startOnBoot = false; // Default value
    m_startMinimized = false; // Default value for new setting
    std::ifstream configFile(m_configFilePath);

    if (!configFile.is_open()) {
        std::cerr << "Info: Could not open config file '" << m_configFilePath << "'. Attempting to create with default settings." << std::endl;
        SaveConfig(); 
        return; 
    }

    std::string line;
    bool foundStartOnBoot = false;
    bool foundStartMinimized = false;
    while (std::getline(configFile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') { continue; }

        size_t separatorPos = line.find('=');
        if (separatorPos != std::string::npos) {
            std::string key = trim(line.substr(0, separatorPos));
            std::string value = trim(line.substr(separatorPos + 1));

            if (!foundStartOnBoot && key == "startOnBoot") {
                m_startOnBoot = (value == "true" || value == "1");
                foundStartOnBoot = true;
            } else if (!foundStartMinimized && key == "startMinimized") {
                m_startMinimized = (value == "true" || value == "1");
                foundStartMinimized = true;
            }
            
            if (foundStartOnBoot && foundStartMinimized) break; 
        }
    }
    configFile.close();
    std::cout << "Loaded config: startOnBoot = " << (m_startOnBoot ? "true" : "false") 
              << ", startMinimized = " << (m_startMinimized ? "true" : "false") << std::endl;
}

void UIStatusLogWindow::SaveConfig() {
    std::ofstream configFile(m_configFilePath); // Opens for writing

    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open config file '" << m_configFilePath << "' for writing." << std::endl;
        return;
    }

    configFile << "# System Configuration" << std::endl;
    configFile << "startOnBoot=" << (m_startOnBoot ? "true" : "false") << std::endl;
    configFile << "startMinimized=" << (m_startMinimized ? "true" : "false") << std::endl;

    configFile.close();

    if (!configFile) { 
         std::cerr << "Error: Failed to write to config file '" << m_configFilePath << "'." << std::endl;
    } else {
        std::cout << "Saved config: startOnBoot = " << (m_startOnBoot ? "true" : "false") 
                  << ", startMinimized = " << (m_startMinimized ? "true" : "false") << std::endl;
    }
}