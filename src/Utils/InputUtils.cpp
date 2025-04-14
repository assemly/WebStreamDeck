#include "InputUtils.hpp"
#include <windows.h> // Include for SendInput and virtual key codes
#include <vector>
#include <string>
#include <algorithm> // For std::sort
#include <imgui_internal.h> // May be needed for specific ImGuiKey details or IsKeyDownMap
#include <set>
#include <chrono> // For timing ESC debounce
#include <iostream> // For cerr

namespace InputUtils {

// Helper function to map ImGuiKey to string representation
// Focuses on non-modifier keys suitable for the main part of a hotkey.
std::string ImGuiKeyToString(ImGuiKey key) {
    // Handle letters and numbers directly
    if (key >= ImGuiKey_A && key <= ImGuiKey_Z) { return std::string(1, (char)('A' + (key - ImGuiKey_A))); }
    if (key >= ImGuiKey_0 && key <= ImGuiKey_9) { return std::string(1, (char)('0' + (key - ImGuiKey_0))); }

    // Function Keys
    if (key >= ImGuiKey_F1 && key <= ImGuiKey_F12) { return "F" + std::to_string(key - ImGuiKey_F1 + 1); }
    // Add F13-F24 if needed (ImGuiKey_F13 etc.)

    // Special Keys
    switch (key) {
        case ImGuiKey_Space:        return "SPACE";
        case ImGuiKey_Enter:        return "ENTER"; // Or RETURN
        case ImGuiKey_Tab:          return "TAB";
        // Note: Escape is handled separately as cancellation
        case ImGuiKey_Backspace:    return "BACKSPACE";
        case ImGuiKey_Delete:       return "DELETE";
        case ImGuiKey_Insert:       return "INSERT";
        case ImGuiKey_Home:         return "HOME";
        case ImGuiKey_End:          return "END";
        case ImGuiKey_PageUp:       return "PAGEUP";
        case ImGuiKey_PageDown:     return "PAGEDOWN";
        case ImGuiKey_LeftArrow:    return "LEFT";
        case ImGuiKey_RightArrow:   return "RIGHT";
        case ImGuiKey_UpArrow:      return "UP";
        case ImGuiKey_DownArrow:    return "DOWN";
        case ImGuiKey_CapsLock:     return "CAPSLOCK";
        case ImGuiKey_NumLock:      return "NUMLOCK";
        case ImGuiKey_ScrollLock:   return "SCROLLLOCK";
        case ImGuiKey_PrintScreen:  return "PRINTSCREEN";

        // Numpad Keys
        case ImGuiKey_Keypad0:      return "NUMPAD0";
        case ImGuiKey_Keypad1:      return "NUMPAD1";
        case ImGuiKey_Keypad2:      return "NUMPAD2";
        case ImGuiKey_Keypad3:      return "NUMPAD3";
        case ImGuiKey_Keypad4:      return "NUMPAD4";
        case ImGuiKey_Keypad5:      return "NUMPAD5";
        case ImGuiKey_Keypad6:      return "NUMPAD6";
        case ImGuiKey_Keypad7:      return "NUMPAD7";
        case ImGuiKey_Keypad8:      return "NUMPAD8";
        case ImGuiKey_Keypad9:      return "NUMPAD9";
        case ImGuiKey_KeypadMultiply: return "MULTIPLY"; // Or NUMPAD*
        case ImGuiKey_KeypadAdd:      return "ADD";      // Or NUMPAD+
        case ImGuiKey_KeypadSubtract: return "SUBTRACT"; // Or NUMPAD-
        case ImGuiKey_KeypadDecimal:  return "DECIMAL";  // Or NUMPAD.
        case ImGuiKey_KeypadDivide:   return "DIVIDE";   // Or NUMPAD/
        case ImGuiKey_KeypadEnter:    return "ENTER";    // Often same as main Enter

        // OEM Keys (Using common US layout names)
        case ImGuiKey_Apostrophe:    return "'";     // ' "
        case ImGuiKey_Comma:         return ",";     // , <
        case ImGuiKey_Minus:         return "-";     // - _
        case ImGuiKey_Period:        return ".";     // . >
        case ImGuiKey_Slash:         return "/";     // / ?
        case ImGuiKey_Semicolon:     return ";";     // ; :
        case ImGuiKey_Equal:         return "=";     // = +
        case ImGuiKey_LeftBracket:   return "[";     // [ {
        case ImGuiKey_Backslash:     return "\\";    // \ |
        case ImGuiKey_RightBracket:  return "]";     // ] }
        case ImGuiKey_GraveAccent:   return "`";     // ` ~

        // Modifiers and unknown keys return empty
        case ImGuiKey_LeftCtrl: case ImGuiKey_RightCtrl:
        case ImGuiKey_LeftShift: case ImGuiKey_RightShift:
        case ImGuiKey_LeftAlt: case ImGuiKey_RightAlt:
        case ImGuiKey_LeftSuper: case ImGuiKey_RightSuper:
        case ImGuiKey_Escape: // Escape is handled separately
        default: return "";
    }
}

static std::set<int> g_pressedKeys;
static char g_hotkeyBuffer[HOTKEY_BUFFER_SIZE] = {0};
static bool g_capturing = false;
static std::chrono::steady_clock::time_point g_lastEscPressTime; // Added for ESC debounce
const std::chrono::milliseconds ESC_DEBOUNCE_DURATION(200); // Debounce ESC for 200ms

bool TryCaptureHotkey(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false; // Invalid buffer
    }

    // 1. Check for cancellation (Escape key)
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { // 'false' means don't repeat
        buffer[0] = '\0'; // Clear the buffer on cancel
        return true;      // Capture finished (cancelled)
    }

    // 2. Get modifier states
    bool ctrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl); // Checks both Left and Right Ctrl
    bool shiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);
    bool altDown = ImGui::IsKeyDown(ImGuiMod_Alt);
    bool superDown = ImGui::IsKeyDown(ImGuiMod_Super); // Win/Cmd key

    // 3. Find the main key *pressed* in this frame
    ImGuiKey mainKeyPressed = ImGuiKey_None;
    for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
         // Skip modifiers, Escape, and unknown keys handled by ImGuiKeyToString returning ""
        if (ImGuiKeyToString(key).empty()) {
            continue;
        }
        // Use IsKeyPressed to detect the initial press, 'false' = don't allow repeat
        if (ImGui::IsKeyPressed(key, false)) {
            mainKeyPressed = key;
            break; // Found the main key pressed this frame
        }
    }

    // 4. If a valid main key was pressed this frame, construct the string
    if (mainKeyPressed != ImGuiKey_None) {
         // Ensure it's not *just* a modifier key being pressed alone
         bool isModifierAlone = (mainKeyPressed == ImGuiKey_LeftCtrl || mainKeyPressed == ImGuiKey_RightCtrl ||
                                mainKeyPressed == ImGuiKey_LeftShift || mainKeyPressed == ImGuiKey_RightShift ||
                                mainKeyPressed == ImGuiKey_LeftAlt || mainKeyPressed == ImGuiKey_RightAlt ||
                                mainKeyPressed == ImGuiKey_LeftSuper || mainKeyPressed == ImGuiKey_RightSuper);
         
        // We need at least one modifier OR the main key itself must be non-modifier
        if (ctrlDown || shiftDown || altDown || superDown || !isModifierAlone) {
            std::string result = "";
            std::vector<std::string> parts;

            // Add modifiers in a consistent order
            if (ctrlDown) parts.push_back("CTRL");
            if (altDown) parts.push_back("ALT");
            if (shiftDown) parts.push_back("SHIFT");
            if (superDown) parts.push_back("WIN"); // Use WIN consistently

            // Add the main key
            std::string mainKeyStr = ImGuiKeyToString(mainKeyPressed);
            if (!mainKeyStr.empty()) {
                 parts.push_back(mainKeyStr);
            } else {
                 // This case should ideally not happen due to the loop condition,
                 // but handle defensively.
                 buffer[0] = '\0';
                 return true; // Treat as invalid combo, finish capture
            }

            // Join parts with "+"
            for (size_t i = 0; i < parts.size(); ++i) {
                result += parts[i];
                if (i < parts.size() - 1) {
                    result += "+";
                }
            }

            // Copy to buffer
            strncpy(buffer, result.c_str(), buffer_size - 1);
            buffer[buffer_size - 1] = '\0'; // Ensure null termination

            return true; // Capture finished successfully
        }
    }

    // No valid combo detected yet, or only modifiers are held down without a main key press this frame
    return false; // Still capturing
}

// Helper to map VK codes to names (simplified)
// ... (existing VkCodeToString may need updates if new keys are added)
const char* VkCodeToString(int vk_code) { 
    // ... (Keep existing mappings) ...
    // Add more mappings if needed for display
    return "Unknown";
}

// Hook procedure (modified slightly for clarity and potential improvements)
// ... (existing LowLevelKeyboardProc can remain largely the same) ...

// --- ADDED: Implementation for simulating media key presses ---
void SimulateMediaKeyPress(WORD vkCode) {
    INPUT inputs[2] = {};

    // Press the key
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vkCode;
    // inputs[0].ki.dwFlags = 0; // 0 for key press

    // Release the key
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vkCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP; // Flag for key release

    // Send the inputs
    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs)) {
        // Handle error if needed (e.g., log a warning)
        // std::cerr << "SendInput failed to send all key events: " << GetLastError() << std::endl;
    }
}

// --- ADDED: Core Audio API Implementation ---
#ifdef _WIN32
// Global pointers for the audio interfaces (simple approach)
IMMDeviceEnumerator *pEnumerator = NULL;
IAudioEndpointVolume *pEndpointVolume = NULL;

// Helper Macro for safe COM release
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

bool InitializeAudioControl() {
    HRESULT hr;
    bool success = false;

    // Initialize COM for this thread
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) { // RPC_E_CHANGED_MODE means COM already initialized differently, which is ok
        std::cerr << "Error: CoInitializeEx failed, HR = 0x" << std::hex << hr << std::endl;
        return false;
    }

    // Get the device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::cerr << "Error: CoCreateInstance(MMDeviceEnumerator) failed, HR = 0x" << std::hex << hr << std::endl;
        CoUninitialize(); // Clean up COM
        return false;
    }

    // Get the default audio endpoint device
    IMMDevice *pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Error: GetDefaultAudioEndpoint failed, HR = 0x" << std::hex << hr << std::endl;
        SAFE_RELEASE(pEnumerator);
        CoUninitialize();
        return false;
    }

    // Activate the IAudioEndpointVolume interface
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr)) {
        std::cerr << "Error: Activate(IAudioEndpointVolume) failed, HR = 0x" << std::hex << hr << std::endl;
    }
    else {
        success = true; // Successfully got the volume interface
    }

    SAFE_RELEASE(pDevice); // Release the device, we have the volume interface now

    // If activation failed, release enumerator and uninitialize COM
    if (!success) {
        SAFE_RELEASE(pEnumerator);
        CoUninitialize();
    }

    std::cout << "Audio control initialized " << (success ? "successfully." : "failed.") << std::endl;
    return success;
}

void UninitializeAudioControl() {
    SAFE_RELEASE(pEndpointVolume);
    SAFE_RELEASE(pEnumerator);
    CoUninitialize(); // Uninitialize COM
    std::cout << "Audio control uninitialized." << std::endl;
}

bool IncreaseMasterVolume() {
    if (!pEndpointVolume) {
        std::cerr << "Error: Audio volume control not initialized." << std::endl;
        return false;
    }
    HRESULT hr = pEndpointVolume->VolumeStepUp(NULL);
    if (FAILED(hr)) {
        _com_error err(hr);
        std::wcerr << L"Error: VolumeStepUp failed: " << err.ErrorMessage() << std::endl;
        return false;
    }
    return true;
}

bool DecreaseMasterVolume() {
    if (!pEndpointVolume) {
        std::cerr << "Error: Audio volume control not initialized." << std::endl;
        return false;
    }
    HRESULT hr = pEndpointVolume->VolumeStepDown(NULL);
     if (FAILED(hr)) {
        _com_error err(hr);
        std::wcerr << L"Error: VolumeStepDown failed: " << err.ErrorMessage() << std::endl;
        return false;
    }
    return true;
}

bool ToggleMasterMute() {
    if (!pEndpointVolume) {
        std::cerr << "Error: Audio volume control not initialized." << std::endl;
        return false;
    }
    BOOL currentMute = FALSE;
    HRESULT hr = pEndpointVolume->GetMute(&currentMute);
    if (FAILED(hr)) {
         _com_error err(hr);
        std::wcerr << L"Error: GetMute failed: " << err.ErrorMessage() << std::endl;
        return false;
    }
    hr = pEndpointVolume->SetMute(!currentMute, NULL);
    if (FAILED(hr)) {
         _com_error err(hr);
        std::wcerr << L"Error: SetMute failed: " << err.ErrorMessage() << std::endl;
        return false;
    }
    return true;
}
#else
// Provide stub implementations for non-Windows platforms
bool InitializeAudioControl() { return false; }
void UninitializeAudioControl() {}
bool IncreaseMasterVolume() { return false; }
bool DecreaseMasterVolume() { return false; }
bool ToggleMasterMute() { return false; }
#endif // _WIN32

} // namespace InputUtils