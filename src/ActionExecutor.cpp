#include "ActionExecutor.hpp"
#include <iostream> // For error reporting
#include <optional>
#include <string> // Needed for wstring conversion
#include <vector> // Needed for wstring conversion buffer

// Platform specific includes for actions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // For ShellExecute
#else
// Add includes for Linux/macOS process creation and URL opening later if needed
#include <cstdlib> // For system()
#endif

// Helper function for string conversion (optional but recommended)
#ifdef _WIN32
std::wstring StringToWstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

ActionExecutor::ActionExecutor(ConfigManager& configManager)
    : m_configManager(configManager)
{
}

bool ActionExecutor::executeAction(const std::string& buttonId)
{
    std::optional<ButtonConfig> buttonOpt = m_configManager.getButtonById(buttonId);
    if (!buttonOpt) {
        std::cerr << "Error: Could not find button configuration for ID: " << buttonId << std::endl;
        return false;
    }

    const ButtonConfig& button = *buttonOpt;
    std::cout << "Executing action for button '" << button.name << "' (ID: " << button.id << ")" << std::endl;
    std::cout << "  Type: " << button.action_type << ", Param: " << button.action_param << std::endl;

    if (button.action_type == "launch_app") {
        return executeLaunchApp(button.action_param);
    } else if (button.action_type == "open_url") {
        return executeOpenUrl(button.action_param);
    } else {
        std::cerr << "Error: Unknown action type: " << button.action_type << std::endl;
        return false;
    }
}

// --- Private Helper Implementations ---

bool ActionExecutor::executeLaunchApp(const std::string& path)
{
#ifdef _WIN32
    // Use ShellExecuteEx for better error handling and options if needed
    // HINSTANCE result = ShellExecute(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    // For simplicity, using CreateProcess which is often more robust for launching apps
    
    STARTUPINFOW si; // Explicitly use STARTUPINFOW
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Convert path to wstring for Windows API
    std::wstring wPath = StringToWstring(path);
    if (wPath.empty() && !path.empty()) { // Handle potential conversion failure
        std::cerr << "Error: Failed to convert path to wstring: " << path << std::endl;
        return false;
    }

    // CreateProcess requires a non-const pointer for lpCommandLine, work with a copy
    std::vector<wchar_t> commandLineVec(wPath.begin(), wPath.end());
    commandLineVec.push_back(0); // Null-terminate

    if (!CreateProcessW(NULL,   // Use CreateProcessW explicitly
                       commandLineVec.data(), // Command line (mutable wchar_t*)
                       NULL,   
                       NULL,   
                       FALSE,  
                       CREATE_NEW_CONSOLE, 
                       NULL,   
                       NULL,    
                       &si,    
                       &pi)    
    )
    {
        DWORD errorCode = GetLastError();
        std::cerr << "Error: Failed to launch application '" << path << "'. CreateProcess failed with error code: " << errorCode << std::endl;
        // You might want to convert errorCode to a human-readable message using FormatMessage
        return false;
    }
    std::cout << "Launched application: " << path << std::endl;
    // Close process and thread handles immediately as we don't need them
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    // Basic implementation for Linux/macOS using system()
    // WARNING: system() is generally discouraged due to security risks.
    // Use fork()/exec() or posix_spawn() in real applications.
    std::string command = path + " &"; // Run in background
    int result = std::system(command.c_str());
    if (result != 0) {
         std::cerr << "Error: Failed to launch application '" << path << "'. system() returned: " << result << std::endl;
         return false;
    }
     std::cout << "Launched application: " << path << std::endl;
    return true;
#endif
}

bool ActionExecutor::executeOpenUrl(const std::string& url)
{
#ifdef _WIN32
    std::wstring wUrl = StringToWstring(url);
     if (wUrl.empty() && !url.empty()) {
        std::cerr << "Error: Failed to convert URL to wstring: " << url << std::endl;
        return false;
    }

    HINSTANCE result = ShellExecuteW(NULL, L"open", wUrl.c_str(), NULL, NULL, SW_SHOWNORMAL); // Use ShellExecuteW and L"open"
    // ShellExecute returns > 32 on success
    if ((intptr_t)result <= 32) {
        std::cerr << "Error: Failed to open URL '" << url << "'. ShellExecute error code: " << (intptr_t)result << std::endl;
        return false;
    }
    std::cout << "Opened URL: " << url << std::endl;
    return true;
#else
    // Basic implementation for Linux/macOS
    std::string command = "xdg-open "; // Linux
    // Add macOS equivalent: command = "open ";
    command += "\"" + url + "\""; // Quote URL
    int result = std::system(command.c_str());
     if (result != 0) {
         std::cerr << "Error: Failed to open URL '" << url << "'. system() returned: " << result << std::endl;
         return false;
    }
    std::cout << "Opened URL: " << url << std::endl;
    return true;
#endif
} 