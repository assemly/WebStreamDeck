#pragma once

#include <string>
#include <optional>

// Only define the class and methods on Windows for now
#ifdef _WIN32

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

private:
    // Private helper functions would go here, e.g.:
    // static HICON GetIconHandle(const std::wstring& filePath);
    // static bool ConvertHICONToRGBA(HICON hIcon, std::vector<unsigned char>& pixels, int& width, int& height);
    // static bool SavePixelsToPng(const unsigned char* pixels, int width, int height, const std::string& outputPath);
};

#endif // _WIN32 