#include "ActionExecutor.hpp"
#include <iostream> // For error reporting
#include <optional>
#include <string> // Needed for wstring conversion
#include <vector> // Needed for wstring conversion buffer & INPUT array
#include <algorithm> // For std::transform
#include <sstream>   // For splitting string
#include <map>       // For key mapping

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

// --- ADDED: Key mapping and SendInput logic ---
#ifdef _WIN32
// Helper function to convert key name string (case-insensitive) to VK code
WORD StringToVkCode(const std::string& keyName) {
    std::string upperKeyName = keyName;
    std::transform(upperKeyName.begin(), upperKeyName.end(), upperKeyName.begin(), ::toupper);

    // Modifier Keys
    if (upperKeyName == "CTRL" || upperKeyName == "CONTROL") return VK_CONTROL;
    if (upperKeyName == "ALT") return VK_MENU; 
    if (upperKeyName == "SHIFT") return VK_SHIFT;
    if (upperKeyName == "WIN" || upperKeyName == "WINDOWS" || upperKeyName == "LWIN") return VK_LWIN; 
    if (upperKeyName == "RWIN") return VK_RWIN; 

    // Function Keys
    if (upperKeyName == "F1") return VK_F1; if (upperKeyName == "F2") return VK_F2;
    if (upperKeyName == "F3") return VK_F3; if (upperKeyName == "F4") return VK_F4;
    if (upperKeyName == "F5") return VK_F5; if (upperKeyName == "F6") return VK_F6;
    if (upperKeyName == "F7") return VK_F7; if (upperKeyName == "F8") return VK_F8;
    if (upperKeyName == "F9") return VK_F9; if (upperKeyName == "F10") return VK_F10;
    if (upperKeyName == "F11") return VK_F11; if (upperKeyName == "F12") return VK_F12;
    
    // Typing Keys (A-Z, 0-9)
    if (upperKeyName.length() == 1) {
        char c = upperKeyName[0];
        if (c >= 'A' && c <= 'Z') return c;
        if (c >= '0' && c <= '9') return c;
    }

    // Special Keys
    if (upperKeyName == "SPACE" || upperKeyName == " ") return VK_SPACE;
    if (upperKeyName == "ENTER" || upperKeyName == "RETURN") return VK_RETURN;
    if (upperKeyName == "TAB") return VK_TAB;
    if (upperKeyName == "ESC" || upperKeyName == "ESCAPE") return VK_ESCAPE;
    if (upperKeyName == "BACKSPACE") return VK_BACK;
    if (upperKeyName == "DELETE" || upperKeyName == "DEL") return VK_DELETE;
    if (upperKeyName == "INSERT" || upperKeyName == "INS") return VK_INSERT;
    if (upperKeyName == "HOME") return VK_HOME;
    if (upperKeyName == "END") return VK_END;
    if (upperKeyName == "PAGEUP" || upperKeyName == "PGUP") return VK_PRIOR; 
    if (upperKeyName == "PAGEDOWN" || upperKeyName == "PGDN") return VK_NEXT; 
    if (upperKeyName == "LEFT") return VK_LEFT;
    if (upperKeyName == "RIGHT") return VK_RIGHT;
    if (upperKeyName == "UP") return VK_UP;
    if (upperKeyName == "DOWN") return VK_DOWN;
    if (upperKeyName == "CAPSLOCK") return VK_CAPITAL;
    if (upperKeyName == "NUMLOCK") return VK_NUMLOCK;
    if (upperKeyName == "SCROLLLOCK") return VK_SCROLL;
    if (upperKeyName == "PRINTSCREEN" || upperKeyName == "PRTSC") return VK_SNAPSHOT;

    // Numpad Keys
    if (upperKeyName == "NUMPAD0") return VK_NUMPAD0; if (upperKeyName == "NUMPAD1") return VK_NUMPAD1;
    if (upperKeyName == "NUMPAD2") return VK_NUMPAD2; if (upperKeyName == "NUMPAD3") return VK_NUMPAD3;
    if (upperKeyName == "NUMPAD4") return VK_NUMPAD4; if (upperKeyName == "NUMPAD5") return VK_NUMPAD5;
    if (upperKeyName == "NUMPAD6") return VK_NUMPAD6; if (upperKeyName == "NUMPAD7") return VK_NUMPAD7;
    if (upperKeyName == "NUMPAD8") return VK_NUMPAD8; if (upperKeyName == "NUMPAD9") return VK_NUMPAD9;
    if (upperKeyName == "MULTIPLY" || upperKeyName == "NUMPAD*") return VK_MULTIPLY;
    if (upperKeyName == "ADD" || upperKeyName == "NUMPAD+") return VK_ADD;
    if (upperKeyName == "SEPARATOR") return VK_SEPARATOR; // Usually locale-specific
    if (upperKeyName == "SUBTRACT" || upperKeyName == "NUMPAD-") return VK_SUBTRACT;
    if (upperKeyName == "DECIMAL" || upperKeyName == "NUMPAD.") return VK_DECIMAL;
    if (upperKeyName == "DIVIDE" || upperKeyName == "NUMPAD/") return VK_DIVIDE;

    // OEM Keys (Common US Layout)
    if (upperKeyName == "+" || upperKeyName == "=") return VK_OEM_PLUS; 
    if (upperKeyName == "-" || upperKeyName == "_") return VK_OEM_MINUS;
    if (upperKeyName == "," || upperKeyName == "<") return VK_OEM_COMMA;
    if (upperKeyName == "." || upperKeyName == ">") return VK_OEM_PERIOD;
    if (upperKeyName == "/" || upperKeyName == "?") return VK_OEM_2; 
    if (upperKeyName == "`" || upperKeyName == "~") return VK_OEM_3; 
    if (upperKeyName == "[" || upperKeyName == "{") return VK_OEM_4; 
    if (upperKeyName == "\\" || upperKeyName == "|") return VK_OEM_5; 
    if (upperKeyName == "]" || upperKeyName == "}") return VK_OEM_6; 
    if (upperKeyName == "'" || upperKeyName == "\"") return VK_OEM_7; // Single/double quote
    if (upperKeyName == ";" || upperKeyName == ":") return VK_OEM_1; // Semicolon/colon

    std::cerr << "Warning: Unknown key name '" << keyName << "'" << std::endl;
    return 0; // Return 0 for unknown keys
}
#endif // _WIN32
// --- End of Key mapping --- 

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
    } else if (button.action_type == "hotkey") {
        #ifdef _WIN32
            return executeHotkey(button.action_param);
        #else
            std::cerr << "Error: Hotkey action is currently only supported on Windows." << std::endl;
            return false;
        #endif
    } else {
        std::cerr << "Error: Unknown action type: " << button.action_type << std::endl;
        return false;
    }
}

// --- ADDED: executeHotkey Implementation ---
#ifdef _WIN32
bool ActionExecutor::executeHotkey(const std::string& keys)
{
    std::vector<WORD> modifierCodes;
    WORD mainKeyCode = 0;

    // Parse the keys string (e.g., "CTRL+ALT+T")
    std::stringstream ss(keys);
    std::string segment;
    while (std::getline(ss, segment, '+')) {
        // Trim whitespace 
        segment.erase(0, segment.find_first_not_of(" \t\r\n"));
        segment.erase(segment.find_last_not_of(" \t\r\n") + 1);

        if (segment.empty()) continue;

        WORD vk = StringToVkCode(segment);
        if (vk == 0) {
             std::cerr << "Error: Invalid key name '" << segment << "' in hotkey string: " << keys << std::endl;
            return false; // Failed to parse
        }

        // Check if it's a modifier key
        if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LWIN || vk == VK_RWIN) {
            // Avoid duplicate modifiers
            bool found = false;
            for(WORD mod : modifierCodes) { if (mod == vk) { found = true; break; } }
            if (!found) modifierCodes.push_back(vk);
        } else {
            if (mainKeyCode != 0) {
                std::cerr << "Error: Multiple non-modifier keys specified in hotkey string: " << keys << " ('" << segment << "' conflicts)" << std::endl;
                return false; // Only one main key allowed
            }
            mainKeyCode = vk;
        }
    }

    if (mainKeyCode == 0) {
        std::cerr << "Error: No main key specified in hotkey string: " << keys << std::endl;
        return false; // Must have a main key
    }

    // Prepare INPUT structures
    size_t numInputs = (modifierCodes.size() + 1) * 2;
    std::vector<INPUT> inputs(numInputs);
    ZeroMemory(inputs.data(), sizeof(INPUT) * numInputs);

    size_t inputIndex = 0;

    // Press Modifiers
    for (WORD modCode : modifierCodes) {
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = modCode;
        // inputs[inputIndex].ki.dwFlags = 0; // Key down (default)
        inputIndex++;
    }

    // Press Main Key
    inputs[inputIndex].type = INPUT_KEYBOARD;
    inputs[inputIndex].ki.wVk = mainKeyCode;
    // inputs[inputIndex].ki.dwFlags = 0; // Key down (default)
    inputIndex++;

    // Release Main Key
    inputs[inputIndex].type = INPUT_KEYBOARD;
    inputs[inputIndex].ki.wVk = mainKeyCode;
    inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP; // Key up
    inputIndex++;

    // Release Modifiers (in reverse order)
    for (auto it = modifierCodes.rbegin(); it != modifierCodes.rend(); ++it) {
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = *it;
        inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP; // Key up
        inputIndex++;
    }

    // Send the inputs
    UINT sent = SendInput(static_cast<UINT>(numInputs), inputs.data(), sizeof(INPUT));
    if (sent != numInputs) {
        std::cerr << "Error: SendInput failed to send all key events. Error code: " << GetLastError() << std::endl;
        // Attempt to release keys that might be stuck (best effort)
        for(size_t i = 0; i < numInputs / 2; ++i) {
            inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &inputs[i], sizeof(INPUT));
        }
        return false;
    }

    std::cout << "Executed hotkey: " << keys << std::endl;
    return true;
}
#endif // _WIN32

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