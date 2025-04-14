#include "ActionExecutor.hpp"
#include "ConfigManager.hpp"
#include "InputUtils.hpp" // Needed for SimulateMediaKeyPress and TryCaptureHotkey
#include <iostream> // For error reporting
#include <optional>
#include <string> // Needed for wstring conversion
#include <vector> // Needed for wstring conversion buffer & INPUT array
#include <algorithm> // For std::transform
#include <sstream>   // For splitting string
#include <map>       // For key mapping
#include <mutex>     // For thread safety

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

// Called from WebSocket thread (or any thread)
void ActionExecutor::requestAction(const std::string& buttonId)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_actionQueue.push(buttonId);
    std::cout << "Queued action request for button ID: " << buttonId << std::endl;
}

// Called from the main thread in the main loop
void ActionExecutor::processPendingActions()
{
    std::string buttonIdToProcess;
    bool actionFound = false;

    // Check queue quickly while locked
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_actionQueue.empty()) {
            buttonIdToProcess = m_actionQueue.front();
            m_actionQueue.pop();
            actionFound = true;
        }
    }

    // Execute action outside the lock
    if (actionFound) {
        executeActionInternal(buttonIdToProcess);
    }
}

// Renamed: Contains the original execution logic, called only from main thread
void ActionExecutor::executeActionInternal(const std::string& buttonId)
{
    auto buttonConfigOpt = m_configManager.getButtonById(buttonId);
    if (!buttonConfigOpt) {
        std::cerr << "Error executing action: Button with ID '" << buttonId << "' not found." << std::endl;
        return;
    }

    const ButtonConfig& config = *buttonConfigOpt;
    const std::string& actionType = config.action_type;
    const std::string& actionParam = config.action_param;

    std::cout << "Executing action for button '" << buttonId 
              << "' (on main thread): Type='" << actionType 
              << "', Param='" << actionParam << "'" << std::endl;

    // --- Action Logic (remains largely the same, but now runs on main thread) ---
    if (actionType == "launch_app") {
        // Using ShellExecute for more flexibility (e.g., opening documents)
        HINSTANCE result = ShellExecuteA(NULL, "open", actionParam.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((intptr_t)result <= 32) { // ShellExecute returns value > 32 on success
            std::cerr << "Error executing launch_app: Failed to launch '" << actionParam << "' (Error code: " << (intptr_t)result << ")" << std::endl;
        }
    } else if (actionType == "open_url") {
        HINSTANCE result = ShellExecuteA(NULL, "open", actionParam.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((intptr_t)result <= 32) {
             std::cerr << "Error executing open_url: Failed to open '" << actionParam << "' (Error code: " << (intptr_t)result << ")" << std::endl;
        }
    } else if (actionType == "hotkey") {
        // --- ADDED: Hotkey Simulation Logic ---
#ifdef _WIN32
        std::vector<WORD> modifierCodes;
        WORD mainKeyCode = 0;

        // Parse the keys string (e.g., "CTRL+ALT+T", "SHIFT+[")
        std::stringstream ss(actionParam);
        std::string segment;
        while (std::getline(ss, segment, '+')) {
            // Trim whitespace 
            segment.erase(0, segment.find_first_not_of(" \t\r\n"));
            segment.erase(segment.find_last_not_of(" \t\r\n") + 1);

            if (segment.empty()) continue;

            WORD vk = StringToVkCode(segment);
            if (vk == 0) {
                 std::cerr << "Error: Invalid key name '" << segment << "' in hotkey string: " << actionParam << std::endl;
                 return; // Stop processing this action
            }

            // Check if it's a modifier key
            if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LWIN || vk == VK_RWIN) {
                // Add modifier (use generic VK codes for simplicity in SendInput)
                WORD generic_vk = vk;
                if(vk == VK_LWIN || vk == VK_RWIN) generic_vk = VK_LWIN; // Treat both as LWIN for press/release
                // Avoid duplicate generic modifiers
                bool found = false;
                for(WORD mod : modifierCodes) { if (mod == generic_vk) { found = true; break; } }
                if (!found) modifierCodes.push_back(generic_vk);
            } else {
                if (mainKeyCode != 0) {
                    std::cerr << "Error: Multiple non-modifier keys specified in hotkey string: " << actionParam << std::endl;
                    return; // Only one main key allowed
                }
                mainKeyCode = vk;
            }
        }

        if (mainKeyCode == 0) {
            std::cerr << "Error: No main key specified in hotkey string: " << actionParam << std::endl;
            return; // Must have a main key
        }

        // Prepare INPUT structures (Max 4 modifiers + 1 main key = 5 pairs)
        const size_t MAX_INPUTS = 10; // 5 down, 5 up
        std::vector<INPUT> inputs(MAX_INPUTS);
        ZeroMemory(inputs.data(), sizeof(INPUT) * MAX_INPUTS);
        size_t inputIndex = 0;

        // Press Modifiers
        for (WORD modCode : modifierCodes) {
            if(inputIndex >= MAX_INPUTS) break; // Bounds check
            inputs[inputIndex].type = INPUT_KEYBOARD;
            inputs[inputIndex].ki.wVk = modCode;
            inputIndex++;
        }

        // Press Main Key
        if(inputIndex >= MAX_INPUTS) return; // Bounds check
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = mainKeyCode;
        inputIndex++;

        // Release Main Key
        if(inputIndex >= MAX_INPUTS) return; // Bounds check
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = mainKeyCode;
        inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP;
        inputIndex++;

        // Release Modifiers (in reverse order)
        for (auto it = modifierCodes.rbegin(); it != modifierCodes.rend(); ++it) {
             if(inputIndex >= MAX_INPUTS) break; // Bounds check
            inputs[inputIndex].type = INPUT_KEYBOARD;
            inputs[inputIndex].ki.wVk = *it;
            inputs[inputIndex].ki.dwFlags = KEYEVENTF_KEYUP;
            inputIndex++;
        }

        // Send the inputs
        if (inputIndex > 0) {
             UINT sent = SendInput(static_cast<UINT>(inputIndex), inputs.data(), sizeof(INPUT));
             if (sent != inputIndex) {
                 std::cerr << "Error: SendInput failed to send all key events for hotkey '" << actionParam << "'. Error code: " << GetLastError() << std::endl;
                 // Attempt to release keys that might be stuck (best effort)
                 for(size_t i = 0; i < inputIndex; ++i) {
                     if (inputs[i].ki.dwFlags == 0) { // If it was a key down
                         inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
                         SendInput(1, &inputs[i], sizeof(INPUT));
                     }
                 }
             } else {
                 std::cout << "Executed hotkey: " << actionParam << std::endl;
             }
        } else {
             std::cerr << "Error: No valid inputs generated for hotkey: " << actionParam << std::endl;
        }
#else
        std::cerr << "Error: Hotkey action is currently only supported on Windows." << std::endl;
#endif
        // --- END Hotkey Simulation Logic ---
    } 
    // --- Handle Media Key Actions (Now running on main thread) ---
    else if (actionType == "media_volume_up") {
        std::cout << "Executing: Media Volume Up (Core Audio) x2" << std::endl;
        // Call the function twice
        bool success1 = InputUtils::IncreaseMasterVolume();
        bool success2 = InputUtils::IncreaseMasterVolume(); 
        if (!success1 || !success2) { // Log if either call failed
             std::cerr << "Failed to increase volume (at least one step failed)." << std::endl;
        }
    }
    else if (actionType == "media_volume_down") {
        // Keep volume down as single step unless requested otherwise
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
    // --- Media keys using SimulateKeyPress (should be safe on main thread too) ---
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
        std::cerr << "Error: Unknown action type '" << actionType << "' for button ID '" << buttonId << "'" << std::endl;
    }
}

// --- executeHotkey Implementation (if exists) should also be called from Internal ---
#ifdef _WIN32
// bool ActionExecutor::executeHotkey(const std::string& keys)
// { ... implementation ... }
#endif

// --- Private Helper Implementations --- 
// bool ActionExecutor::executeLaunchApp(const std::string& path)
// { ... implementation ... }

// bool ActionExecutor::executeOpenUrl(const std::string& url)
// { ... implementation ... } 