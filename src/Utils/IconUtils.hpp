#pragma once

#include <string>
#include <optional>
#include <vector>

// Only define the class and methods on Windows for now
#ifdef _WIN32

#include <windows.h> // Include for HICON type

// Structure to hold raw image data
struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels; // RGBA format
};

class IconUtils {
public:
    // Attempts to extract an icon from an executable (.exe) or shortcut (.lnk),
    // save it as a PNG to the specified directory using the desired base name.
    // Returns the full path to the saved PNG file on success, or std::nullopt on failure.
    static std::optional<std::string> ExtractAndSaveIconPng(
        const std::wstring& filePath,      // Input file path (.exe or .lnk)
        const std::string& outputDir,     // Directory to save the PNG (e.g., "assets/icons")
        const std::string& desiredBaseName // Base name for the output PNG (e.g., "btn_myapp")
    );

    // Converts an HICON handle to raw RGBA pixel data.
    // Returns ImageData on success, or std::nullopt on failure.
    // IMPORTANT: The caller does NOT own the input hIcon and should NOT destroy it.
    // This function only reads from the hIcon.
    static std::optional<ImageData> ConvertHIconToRGBA(HICON hIcon);

private:
    // Private helper functions are now less needed as ConvertHIconToRGBA is public static
    // static HICON GetIconHandle(const std::wstring& filePath);
};

#endif // _WIN32 