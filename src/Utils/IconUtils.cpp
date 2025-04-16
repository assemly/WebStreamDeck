#include "IconUtils.hpp"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // For ExtractIconExW, maybe SHGetFileInfoW
#include <vector>     // For pixel buffer
#include <iostream>   // For error logging
#include <filesystem> // For path manipulation
#include <cstdio>     // For FILE, fopen, fwrite, fclose

// Define STB implementation *before* including stb_image_write.h
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace fs = std::filesystem;

// Need WideToUtf8 helper for logging
#include "StringUtils.hpp" // Assume WideToUtf8/ToLowerW declarations are here

// Define the callback function for stbi_write_png_to_func
static void stbi_write_callback(void *context, void *data, int size) {
    fwrite(data, 1, size, (FILE*)context);
}

std::optional<std::string> IconUtils::ExtractAndSaveIconPng(
    const std::wstring& filePath,
    const std::string& outputDir,
    const std::string& desiredBaseName)
{
    std::cout << "[IconUtils] Attempting to extract icon for '" << desiredBaseName << "' from: " 
              << StringUtils::WideToUtf8(filePath) << std::endl;

    // 1. Check/Create Output Directory
    try {
        if (!fs::exists(outputDir)) {
            if (!fs::create_directories(outputDir)) {
                std::cerr << "[IconUtils] Error: Failed to create output directory: " << outputDir << std::endl;
                return std::nullopt;
            }
            std::cout << "[IconUtils] Created output directory: " << outputDir << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[IconUtils] Filesystem error checking/creating directory: " << e.what() << std::endl;
        return std::nullopt;
    }

    // 2. Construct Full Output Path using generic_string for consistent separators
    fs::path fullPath = fs::path(outputDir) / (desiredBaseName + ".png");
    std::string outputPathUtf8 = fullPath.generic_string(); // UTF-8 for return/logging
    std::cout << "[IconUtils] Target output path (generic UTF-8): " << outputPathUtf8 << std::endl;

    // --- TODO: Implementation Steps ---
    HICON hIcon = NULL;
    int iconWidth = 0;
    int iconHeight = 0;
    std::vector<unsigned char> pixels;

    // 3. Extract HICON (Start with .exe only)
    // TODO: Add .lnk handling later
    fs::path inputPath(filePath);
    std::wstring extension = inputPath.has_extension() ? StringUtils::ToLowerW(inputPath.extension().wstring()) : L"";

    if (extension == L".exe") {
        UINT iconCount = ExtractIconExW(filePath.c_str(), -1, NULL, NULL, 0);
        if (iconCount > 0) {
            // Try to extract the first (usually largest) icon
            if (ExtractIconExW(filePath.c_str(), 0, &hIcon, NULL, 1) != 1) {
                hIcon = NULL; // Ensure hIcon is NULL if extraction failed
            }
        } else {
            std::cerr << "[IconUtils] No icons found in executable: " << StringUtils::WideToUtf8(filePath) << std::endl;
        }
    } else if (extension == L".lnk") {
         std::cerr << "[IconUtils] .lnk file icon extraction not implemented yet." << std::endl;
         // TODO: Parse LNK, get target, call recursively or extract from LNK itself
    } else {
         std::cerr << "[IconUtils] Unsupported file type for icon extraction: " << StringUtils::WideToUtf8(filePath) << std::endl;
    }
    
    if (!hIcon) {
        std::cerr << "[IconUtils] Failed to extract icon handle (HICON)." << std::endl;
        return std::nullopt;
    }
    std::cout << "[IconUtils] Successfully extracted icon handle (HICON)." << std::endl;

    // --- Step 4: Convert HICON to RGBA pixel data ---
    ICONINFO iconInfo = {0};
    BITMAP bmpColor = {0};
    BITMAP bmpMask = {0};
    std::vector<unsigned char> colorPixels;
    std::vector<unsigned char> maskPixels;
    bool success = false;

    if (GetIconInfo(hIcon, &iconInfo)) {
        if (iconInfo.hbmColor) {
            GetObject(iconInfo.hbmColor, sizeof(bmpColor), &bmpColor);
            iconWidth = bmpColor.bmWidth;
            iconHeight = bmpColor.bmHeight;

            // Get color bitmap pixels (usually 32-bit BGRA)
            colorPixels.resize(iconWidth * iconHeight * 4);
            HDC hdcScreen = GetDC(NULL);

            // Create and populate a proper BITMAPINFO structure
            BITMAPINFO bmiColor = {0};
            bmiColor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmiColor.bmiHeader.biWidth = iconWidth;
            bmiColor.bmiHeader.biHeight = -iconHeight; // Request top-down DIB
            bmiColor.bmiHeader.biPlanes = 1;
            bmiColor.bmiHeader.biBitCount = 32; // Request 32-bit pixels (BGRA)
            bmiColor.bmiHeader.biCompression = BI_RGB;

            if (GetDIBits(hdcScreen, iconInfo.hbmColor, 0, iconHeight, colorPixels.data(), &bmiColor, DIB_RGB_COLORS) > 0) {
                // Got color pixels. Now get mask if it exists (it should for icons)
                if (iconInfo.hbmMask) {
                    GetObject(iconInfo.hbmMask, sizeof(bmpMask), &bmpMask);
                    // Mask is usually 1-bit per pixel, height might be doubled if XOR mask present (not handled here simply)
                    // We only need the basic AND mask part typically stored with hbmMask
                    maskPixels.resize(iconWidth * iconHeight); // Assuming 1 bit per pixel mask
                    // Create a compatible DC and select the mask bitmap to query its bits
                    HDC hdcMem = CreateCompatibleDC(hdcScreen);
                    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconInfo.hbmMask);
                    BITMAPINFO bmiMask = {0};
                    bmiMask.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmiMask.bmiHeader.biWidth = iconWidth;
                    bmiMask.bmiHeader.biHeight = -iconHeight; // Top-down DIB
                    bmiMask.bmiHeader.biPlanes = 1;
                    bmiMask.bmiHeader.biBitCount = 1; // Request 1-bit mask data
                    bmiMask.bmiHeader.biCompression = BI_RGB;

                    // Query size needed for mask bits
                    GetDIBits(hdcMem, iconInfo.hbmMask, 0, iconHeight, NULL, &bmiMask, DIB_RGB_COLORS);
                    DWORD maskSize = bmiMask.bmiHeader.biSizeImage;
                    if(maskSize > 0) {
                        maskPixels.resize(maskSize);
                        if (GetDIBits(hdcMem, iconInfo.hbmMask, 0, iconHeight, maskPixels.data(), &bmiMask, DIB_RGB_COLORS) > 0) {
                            // Now combine colorPixels (BGRA) and maskPixels (1-bit) into final RGBA
                            pixels.resize(iconWidth * iconHeight * 4);
                            for (int y = 0; y < iconHeight; ++y) {
                                for (int x = 0; x < iconWidth; ++x) {
                                    int pixelIndex = (y * iconWidth + x);
                                    int rgbaIndex = pixelIndex * 4;
                                    int bgraIndex = rgbaIndex; // Assuming BGRA format from GetDIBits

                                    // Get the mask bit (masks are often padded per line)
                                    int maskByteIndex = (y * ((iconWidth + 31) / 32 * 4)) + (x / 8); // Calculate byte index, considering padding
                                    int maskBitIndex = 7 - (x % 8); // Bit index within the byte (MSB first)
                                    bool isTransparent = (maskByteIndex < maskPixels.size()) && ((maskPixels[maskByteIndex] >> maskBitIndex) & 1);
                                    
                                    // Copy BGR, set Alpha based on mask
                                    pixels[rgbaIndex + 0] = colorPixels[bgraIndex + 2]; // Blue -> Red
                                    pixels[rgbaIndex + 1] = colorPixels[bgraIndex + 1]; // Green -> Green
                                    pixels[rgbaIndex + 2] = colorPixels[bgraIndex + 0]; // Red -> Blue
                                    pixels[rgbaIndex + 3] = isTransparent ? 0 : 255;  // Alpha
                                }
                            }
                            success = true;
                            std::cout << "[IconUtils] Successfully converted HICON to RGBA pixels (" << iconWidth << "x" << iconHeight << ")." << std::endl;
                        } else { std::cerr << "[IconUtils] GetDIBits failed for mask bitmap." << std::endl; }
                    } else { std::cerr << "[IconUtils] Calculated mask size is zero." << std::endl; }

                    SelectObject(hdcMem, hbmOld); // Deselect bitmap
                    DeleteDC(hdcMem);
                } else { std::cerr << "[IconUtils] IconInfo missing mask bitmap (hbmMask)." << std::endl; }
            } else { std::cerr << "[IconUtils] GetDIBits failed for color bitmap. Error code: " << GetLastError() << std::endl; }
            ReleaseDC(NULL, hdcScreen);
        } else { std::cerr << "[IconUtils] IconInfo missing color bitmap (hbmColor)." << std::endl; }

        // Clean up GDI objects obtained from GetIconInfo
        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    } else {
        std::cerr << "[IconUtils] GetIconInfo failed for the icon handle." << std::endl;
    }

    // --- Step 5: Save RGBA pixels to PNG using wide char file access ---
    std::optional<std::string> resultPath = std::nullopt;
#ifdef _WIN32
    if (success) {
        // Convert UTF-8 path to wide string for Windows file API
        std::wstring wOutputPath = StringUtils::Utf8ToWide(outputPathUtf8);
        if (wOutputPath.empty() && !outputPathUtf8.empty()) {
            std::cerr << "[IconUtils] Failed to convert output path to wstring: " << outputPathUtf8 << std::endl;
        } else {
            FILE* fp = nullptr;
            errno_t err = _wfopen_s(&fp, wOutputPath.c_str(), L"wb"); // Use secure version

            if (err == 0 && fp != nullptr) {
                std::cout << "[IconUtils] Successfully opened file for writing (wide): " << outputPathUtf8 << std::endl;
                // Use stbi_write_png_to_func with the FILE* as context
                int write_result = stbi_write_png_to_func(stbi_write_callback, fp, iconWidth, iconHeight, 4, pixels.data(), iconWidth * 4);
                
                fclose(fp); // Close the file regardless of write result

                if (write_result) {
                    std::cout << "[IconUtils] Successfully saved icon using stbi_write_png_to_func to: " << outputPathUtf8 << std::endl;
                    resultPath = outputPathUtf8; // Return the original UTF-8 path
                } else {
                    std::cerr << "[IconUtils] Failed to write PNG data using stbi_write_png_to_func to: " << outputPathUtf8 << std::endl;
                    // Optionally delete the potentially incomplete file?
                    // _wremove(wOutputPath.c_str());
                }
            } else {
                std::cerr << "[IconUtils] Failed to open file for writing (wide) using _wfopen_s. Path: " 
                          << outputPathUtf8 << ", Error code: " << err << std::endl;
            }
        }
    }
#else
    // Non-Windows implementation (assuming stbi_write_png handles UTF-8 paths correctly)
    if (success) {
        if (stbi_write_png(outputPathUtf8.c_str(), iconWidth, iconHeight, 4, pixels.data(), iconWidth * 4)) {
            std::cout << "[IconUtils] Successfully saved icon to: " << outputPathUtf8 << std::endl;
            resultPath = outputPathUtf8;
        } else {
            std::cerr << "[IconUtils] Failed to write PNG file using stb_image_write to: " << outputPathUtf8 << std::endl;
        }
    }
#endif

    // --- Step 6: Cleanup HICON ---
    DestroyIcon(hIcon);
    hIcon = NULL;

    // --- Step 7: Return Result ---
    return resultPath;
}

// --- TODO: Implement private helper functions --- 

#endif // _WIN32 