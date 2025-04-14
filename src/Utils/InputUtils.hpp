#pragma once
#include <string>
#include <imgui.h> // Required for ImGuiKey

// ADDED: Include windows.h specifically for WORD definition under WIN32
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude less common parts of windows.h
#include <windows.h>
// ADDED: Include Core Audio API headers
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <comdef.h> // For _com_error reporting (optional)
#endif

// Define HOTKEY_BUFFER_SIZE if not already defined somewhere else
#ifndef HOTKEY_BUFFER_SIZE
#define HOTKEY_BUFFER_SIZE 256
#endif

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

    // ADDED: Declaration for simulating media key presses
    // Ensures WORD is defined before use (under WIN32)
    void SimulateMediaKeyPress(WORD vkCode);

    // ADDED: Direct Volume Control using Core Audio API
    bool InitializeAudioControl();   // Call once at startup maybe?
    void UninitializeAudioControl(); // Call once at shutdown maybe?
    bool IncreaseMasterVolume();
    bool DecreaseMasterVolume();
    bool ToggleMasterMute();

} // namespace InputUtils