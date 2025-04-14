#pragma once
#include <string>
#include <imgui.h> // Required for ImGuiKey

namespace InputUtils {
    // Converts ImGuiKey enum to a display string (e.g., ImGuiKey_A -> "A")
    // Returns empty string for unknown or modifier keys handled separately.
    std::string ImGuiKeyToString(ImGuiKey key);

    // Checks for hotkey input in the current frame.
    // If a valid combination is pressed, formats it into the buffer and returns true.
    // If Escape is pressed, clears the buffer and returns true (capture cancelled).
    // Otherwise, returns false (still capturing or no valid input yet).
    // buffer: The char buffer to store the formatted hotkey string.
    // buffer_size: The size of the buffer.
    // returns: True if capture finished (valid combo or Esc pressed), false otherwise.
    bool TryCaptureHotkey(char* buffer, size_t buffer_size);

} // namespace InputUtils