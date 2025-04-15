#include "ActionExecutionService.hpp"
#include "../Utils/InputUtils.hpp" // Needed for SimulateMediaKeyPress and audio
#include <iostream> // For error reporting
#include <string> // Needed for wstring conversion, etc.
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

// --- Helper Functions (Moved from ActionExecutor.cpp) --- 

#ifdef _WIN32
// Helper function for string conversion to wstring
std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

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

// --- ActionExecutionService Implementation --- 

// Constructor (doesn't need ConfigManager anymore)
ActionExecutionService::ActionExecutionService() {}

// Executes an action based on explicit type and parameter
void ActionExecutionService::executeAction(const std::string& actionType, const std::string& actionParam) {
    std::cout << "[Exec Service] Executing action: Type='" << actionType 
              << "', Param='" << actionParam << "'" << std::endl;

    // --- Action Logic (using actionType and actionParam directly) ---
    if (actionType == "launch_app") {
        executeLaunchApp(actionParam);
    } else if (actionType == "open_url") {
        executeOpenUrl(actionParam);
    } else if (actionType == "hotkey") {
        executeHotkey(actionParam);
    } 
    // --- Media Key Actions --- 
    else if (actionType == "media_volume_up") {
        std::cout << "Executing: Media Volume Up (Core Audio) x2" << std::endl;
        bool success1 = InputUtils::IncreaseMasterVolume();
        bool success2 = InputUtils::IncreaseMasterVolume(); 
        if (!success1 || !success2) {
             std::cerr << "Failed to increase volume (at least one step failed)." << std::endl;
        }
    }
    else if (actionType == "media_volume_down") {
        std::cout << "Executing: Media Volume Down (Core Audio)" << std::endl;
        if (!InputUtils::DecreaseMasterVolume()) {
             std::cerr << "Failed to decrease volume." << std::endl;
        }
    }
    else if (actionType == "media_mute") {
        std::cout << "Executing: Media Mute Toggle (Core Audio)" << std::endl;
        if (!InputUtils::ToggleMasterMute()) {
             std::cerr << "Failed to toggle mute." << std::endl;
        }
    }
    else if (actionType == "media_play_pause") {
        std::cout << "Executing: Media Play/Pause (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_PLAY_PAUSE);
    }
    else if (actionType == "media_next_track") {
        std::cout << "Executing: Media Next Track (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_NEXT_TRACK);
    }
    else if (actionType == "media_prev_track") {
        std::cout << "Executing: Media Previous Track (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_PREV_TRACK);
    }
    else if (actionType == "media_stop") {
        std::cout << "Executing: Media Stop (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_STOP);
    }
    // --- END of Media Key Actions ---
    else {
        std::cerr << "Error: Unknown action type '" << actionType << "' requested." << std::endl;
    }
}

// --- Private Helper Method Implementations (Moved from ActionExecutor.cpp) --- 

bool ActionExecutionService::executeLaunchApp(const std::string& path) {
#ifdef _WIN32
    HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result <= 32) { 
        std::cerr << "Error executing launch_app: Failed to launch '" << path << "' (Error code: " << (intptr_t)result << ")" << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "Error: launch_app action is currently only supported on Windows." << std::endl;
    // Example for Linux/macOS (less robust):
    // std::string command = "xdg-open " + path; // Linux
    // std::string command = "open " + path; // macOS
    // int result = system(command.c_str());
    // return result == 0;
    return false;
#endif
}

bool ActionExecutionService::executeOpenUrl(const std::string& url) {
#ifdef _WIN32
    HINSTANCE result = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result <= 32) {
         std::cerr << "Error executing open_url: Failed to open '" << url << "' (Error code: " << (intptr_t)result << ")" << std::endl;
         return false;
    }
    return true;
#else
    std::cerr << "Error: open_url action is currently only supported on Windows." << std::endl;
    // Example for Linux/macOS:
    // std::string command = "xdg-open " + url; // Linux
    // std::string command = "open " + url; // macOS
    // int result = system(command.c_str());
    // return result == 0;
    return false;
#endif
}

bool ActionExecutionService::executeHotkey(const std::string& keys) {
#ifdef _WIN32
    std::vector<WORD> modifierCodes;
    WORD mainKeyCode = 0;

    // Parse the keys string (e.g., "CTRL+ALT+T")
    std::stringstream ss(keys);
    std::string segment;
    while (std::getline(ss, segment, '+')) {
        segment.erase(0, segment.find_first_not_of(" \t\r\n"));
        segment.erase(segment.find_last_not_of(" \t\r\n") + 1);
        if (segment.empty()) continue;

        WORD vk = StringToVkCode(segment); // Use helper function
        if (vk == 0) {
             std::cerr << "Error: Invalid key name '" << segment << "' in hotkey string: " << keys << std::endl;
             return false; 
        }
        if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LWIN || vk == VK_RWIN) {
            WORD generic_vk = (vk == VK_RWIN) ? VK_LWIN : vk; // Normalize RWIN to LWIN for press/release
            bool found = false;
            for(WORD mod : modifierCodes) { if (mod == generic_vk) { found = true; break; } }
            if (!found) modifierCodes.push_back(generic_vk);
        } else {
            if (mainKeyCode != 0) {
                std::cerr << "Error: Multiple non-modifier keys specified in hotkey string: " << keys << std::endl;
                return false; 
            }
            mainKeyCode = vk;
        }
    }

    if (mainKeyCode == 0) {
        std::cerr << "Error: No main key specified in hotkey string: " << keys << std::endl;
        return false; 
    }

    // Prepare INPUT structures
    const size_t MAX_INPUTS = 10; 
    std::vector<INPUT> inputs(MAX_INPUTS);
    ZeroMemory(inputs.data(), sizeof(INPUT) * MAX_INPUTS);
    size_t inputIndex = 0;

    // Press Modifiers
    for (WORD modCode : modifierCodes) {
        if(inputIndex >= MAX_INPUTS) { std::cerr << "Hotkey too complex." << std::endl; return false; } 
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = modCode;
        inputIndex++;
    }
    // Press Main Key
    if(inputIndex >= MAX_INPUTS) { std::cerr << "Hotkey too complex." << std::endl; return false; } 
    inputs[inputIndex].type = INPUT_KEYBOARD;
    inputs[inputIndex].ki.wVk = mainKeyCode;
    inputIndex++;
    // Release Main Key
    if(inputIndex >= MAX_INPUTS) { std::cerr << "Hotkey too complex." << std::endl; return false; } 
    inputs[inputIndex].type = INPUT_KEYBOARD;
    inputs[inputIndex].ki.wVk = mainKeyCode;
    inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP;
    inputIndex++;
    // Release Modifiers (reverse)
    for (auto it = modifierCodes.rbegin(); it != modifierCodes.rend(); ++it) {
         if(inputIndex >= MAX_INPUTS) { std::cerr << "Hotkey too complex." << std::endl; return false; } 
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = *it;
        inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP;
        inputIndex++;
    }

    // Send the inputs
    if (inputIndex > 0) {
         UINT sent = SendInput(static_cast<UINT>(inputIndex), inputs.data(), sizeof(INPUT));
         if (sent != inputIndex) {
             std::cerr << "Error: SendInput failed for hotkey '" << keys << "'. Error: " << GetLastError() << std::endl;
             // Best effort cleanup might be needed here
             return false;
         } else {
             std::cout << "Executed hotkey: " << keys << std::endl;
             return true;
         }
    } else {
         std::cerr << "Error: No inputs generated for hotkey: " << keys << std::endl;
         return false;
    }
#else
    std::cerr << "Error: Hotkey action is currently only supported on Windows." << std::endl;
    return false;
#endif
} 