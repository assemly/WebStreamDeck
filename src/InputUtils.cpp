#include "InputUtils.hpp"
#include <vector>
#include <string>
#include <algorithm> // For std::sort
#include <imgui_internal.h> // May be needed for specific ImGuiKey details or IsKeyDownMap

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


} // namespace InputUtils