#include "IconUtils.hpp"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // For ExtractIconExW
#include <shlobj.h>   // For IShellLinkW, IPersistFile, CoInitializeEx, CoUninitialize
#include <objbase.h>  // For CoCreateInstance
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

// RAII wrapper for COM initialization/uninitialization
class CoInitializer {
public:
    CoInitializer() : hr(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {
        if (FAILED(hr)) {
            std::cerr << "[IconUtils] CoInitializeEx failed, HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
    }
    ~CoInitializer() {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
    HRESULT GetHResult() const { return hr; }
private:
    HRESULT hr;
};


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


    HICON hIcon = NULL;
    int iconWidth = 0;
    int iconHeight = 0;
    std::vector<unsigned char> pixels;

    // --- Step 3: Extract HICON based on file type ---
    fs::path inputPath(filePath);
    std::wstring extension = inputPath.has_extension() ? StringUtils::ToLowerW(inputPath.extension().wstring()) : L"";

    if (extension == L".exe") {
        std::cout << "[IconUtils] Input is an EXE file." << std::endl;
        UINT iconCount = ExtractIconExW(filePath.c_str(), -1, NULL, NULL, 0);
        if (iconCount > 0) {
            // Try to extract the first (usually largest) icon
            if (ExtractIconExW(filePath.c_str(), 0, &hIcon, NULL, 1) != 1) {
                 std::cerr << "[IconUtils] ExtractIconExW failed for index 0 on EXE." << std::endl;
                hIcon = NULL; // Ensure hIcon is NULL if extraction failed
            } else {
                std::cout << "[IconUtils] Successfully extracted icon (index 0) from EXE." << std::endl;
            }
        } else {
            std::cerr << "[IconUtils] No icons found in executable: " << StringUtils::WideToUtf8(filePath) << std::endl;
        }
    } else if (extension == L".lnk") {
        std::cout << "[IconUtils] Input is a LNK file. Initializing COM..." << std::endl;
        CoInitializer comInit; // Initialize COM for this scope
        if (FAILED(comInit.GetHResult())) {
             std::cerr << "[IconUtils] COM initialization failed. Cannot process LNK file." << std::endl;
             // No need to CoUninitialize as RAII handles it
             return std::nullopt;
        }
        std::cout << "[IconUtils] COM initialized successfully." << std::endl;

        IShellLinkW* psl = NULL;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);

        if (SUCCEEDED(hr)) {
            std::cout << "[IconUtils] IShellLinkW instance created." << std::endl;
            IPersistFile* ppf = NULL;
            hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
            if (SUCCEEDED(hr)) {
                std::cout << "[IconUtils] IPersistFile interface obtained." << std::endl;
                hr = ppf->Load(filePath.c_str(), STGM_READ); // Load the shortcut file
                if (SUCCEEDED(hr)) {
                    std::cout << "[IconUtils] LNK file loaded successfully." << std::endl;
                    // Optional: Resolve the link just in case
                    // hr = psl->Resolve(NULL, SLR_NO_UI | SLR_UPDATE); // hwnd can be NULL

                    // Get the icon location specified in the shortcut
                    wchar_t szIconPath[MAX_PATH] = {0};
                    int iIconIndex = 0;
                    hr = psl->GetIconLocation(szIconPath, MAX_PATH, &iIconIndex);

                    if (SUCCEEDED(hr) && szIconPath[0] != L'\0') {
                         std::wcout << L"[IconUtils] LNK GetIconLocation succeeded. Icon Path: " << szIconPath << L", Index: " << iIconIndex << std::endl;
                         // Expand environment variables in path, just in case
                         wchar_t szExpandedPath[MAX_PATH];
                         if (ExpandEnvironmentStringsW(szIconPath, szExpandedPath, MAX_PATH) > 0) {
                            UINT iconsExtracted = ExtractIconExW(szExpandedPath, iIconIndex, &hIcon, NULL, 1);
                            if (iconsExtracted == 1) {
                                std::wcout << L"[IconUtils] Successfully extracted icon from LNK's specified location: " << szExpandedPath << L", index " << iIconIndex << std::endl;
                            } else {
                                std::wcerr << L"[IconUtils] ExtractIconExW failed for LNK's specified location: " << szExpandedPath << L", index " << iIconIndex << L", Error: " << GetLastError() << std::endl;
                                hIcon = NULL;
                                // Fallback: Try index 0 from the same path if specific index failed
                                if (iIconIndex != 0) {
                                    iconsExtracted = ExtractIconExW(szExpandedPath, 0, &hIcon, NULL, 1);
                                    if(iconsExtracted == 1) {
                                        std::wcout << L"[IconUtils] Fallback: Successfully extracted icon index 0 from: " << szExpandedPath << std::endl;
                                    } else {
                                         std::wcerr << L"[IconUtils] Fallback failed: ExtractIconExW index 0 from: " << szExpandedPath << L", Error: " << GetLastError() << std::endl;
                                        hIcon = NULL;
                                    }
                                }
                            }
                         } else {
                             std::wcerr << L"[IconUtils] Failed to expand environment strings for icon path: " << szIconPath << std::endl;
                         }
                    } else {
                         std::cerr << "[IconUtils] LNK GetIconLocation failed or returned empty path. HRESULT: 0x" << std::hex << hr << std::dec << ". Attempting to get target path..." << std::endl;
                        // GetIconLocation failed or no specific icon set, try getting icon from the target path
                        wchar_t szTargetPath[MAX_PATH] = {0};
                        hr = psl->GetPath(szTargetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY); // Get target path
                        if (SUCCEEDED(hr) && szTargetPath[0] != L'\0') {
                            std::wcout << L"[IconUtils] LNK GetPath succeeded. Target Path: " << szTargetPath << std::endl;
                            // Expand environment strings
                            wchar_t szExpandedTargetPath[MAX_PATH];
                            if (ExpandEnvironmentStringsW(szTargetPath, szExpandedTargetPath, MAX_PATH) > 0) {
                                UINT iconsExtracted = ExtractIconExW(szExpandedTargetPath, 0, &hIcon, NULL, 1); // Try default icon index 0
                                if (iconsExtracted == 1) {
                                    std::wcout << L"[IconUtils] Successfully extracted default icon (index 0) from LNK target: " << szExpandedTargetPath << std::endl;
                                } else {
                                    std::wcerr << L"[IconUtils] ExtractIconExW failed for LNK target (index 0): " << szExpandedTargetPath << L", Error: " << GetLastError() << std::endl;
                                    hIcon = NULL;
                                }
                            } else {
                                std::wcerr << L"[IconUtils] Failed to expand environment strings for target path: " << szTargetPath << std::endl;
                            }
                        } else {
                            std::cerr << "[IconUtils] LNK GetPath failed or returned empty path. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
                        }
                    }
                } else {
                     std::cerr << "[IconUtils] IPersistFile::Load failed. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
                }
                ppf->Release();
                std::cout << "[IconUtils] IPersistFile released." << std::endl;
            } else {
                std::cerr << "[IconUtils] QueryInterface for IPersistFile failed. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
            }
            psl->Release();
             std::cout << "[IconUtils] IShellLinkW released." << std::endl;
        } else {
            std::cerr << "[IconUtils] CoCreateInstance for CLSID_ShellLink failed. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
        // CoUninitialize is handled by CoInitializer destructor
         std::cout << "[IconUtils] COM scope finished for LNK handling." << std::endl;
    } else {
         std::cerr << "[IconUtils] Unsupported file type for icon extraction: " << StringUtils::WideToUtf8(filePath) << std::endl;
    }

    if (!hIcon) {
        std::cerr << "[IconUtils] Failed to obtain a valid icon handle (HICON) after processing." << std::endl;
        return std::nullopt; // Return early if no icon handle was obtained
    }
    std::cout << "[IconUtils] Successfully obtained icon handle (HICON)." << std::endl;

    // --- Step 4: Convert HICON to RGBA pixel data ---
    ICONINFO iconInfo = {0};
    BITMAP bmpColor = {0};
    BITMAP bmpMask = {0};
    std::vector<unsigned char> colorPixels;
    std::vector<unsigned char> maskPixels;
    bool success = false;

    if (GetIconInfo(hIcon, &iconInfo)) {
        // Wrap GDI objects for automatic cleanup
        struct GdiObjectDeleter { void operator()(HGDIOBJ obj) const { if (obj) DeleteObject(obj); } };
        std::unique_ptr<void, GdiObjectDeleter> hbmColorGuard(iconInfo.hbmColor);
        std::unique_ptr<void, GdiObjectDeleter> hbmMaskGuard(iconInfo.hbmMask);

        if (iconInfo.hbmColor) {
            GetObject(iconInfo.hbmColor, sizeof(bmpColor), &bmpColor);
            iconWidth = bmpColor.bmWidth;
            iconHeight = bmpColor.bmHeight;
             std::cout << "[IconUtils] Icon dimensions: " << iconWidth << "x" << iconHeight << std::endl;

            // Get color bitmap pixels (usually 32-bit BGRA)
            colorPixels.resize(iconWidth * iconHeight * 4);
            HDC hdcScreen = GetDC(NULL);
            if (!hdcScreen) {
                 std::cerr << "[IconUtils] GetDC(NULL) failed." << std::endl;
                 DestroyIcon(hIcon);
                 return std::nullopt;
            }

            // Create and populate a proper BITMAPINFO structure
            BITMAPINFO bmiColor = {0};
            bmiColor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmiColor.bmiHeader.biWidth = iconWidth;
            bmiColor.bmiHeader.biHeight = -iconHeight; // Request top-down DIB
            bmiColor.bmiHeader.biPlanes = 1;
            bmiColor.bmiHeader.biBitCount = 32; // Request 32-bit pixels (BGRA)
            bmiColor.bmiHeader.biCompression = BI_RGB;

            if (GetDIBits(hdcScreen, (HBITMAP)iconInfo.hbmColor, 0, iconHeight, colorPixels.data(), &bmiColor, DIB_RGB_COLORS) > 0) {
                std::cout << "[IconUtils] Successfully got color DIBits." << std::endl;
                // Got color pixels. Now get mask if it exists (it should for icons)
                if (iconInfo.hbmMask) {
                     std::cout << "[IconUtils] Mask bitmap (hbmMask) exists. Getting mask info..." << std::endl;
                    GetObject(iconInfo.hbmMask, sizeof(bmpMask), &bmpMask);
                     std::cout << "[IconUtils] Mask dimensions: " << bmpMask.bmWidth << "x" << bmpMask.bmHeight << std::endl;
                     if (bmpMask.bmWidth != iconWidth || abs(bmpMask.bmHeight) != iconHeight) {
                        std::cerr << "[IconUtils] Warning: Mask dimensions (" << bmpMask.bmWidth << "x" << bmpMask.bmHeight
                                  << ") do not match color dimensions (" << iconWidth << "x" << iconHeight << "). Adjusting expected mask size." << std::endl;
                        // We proceed assuming the mask corresponds somehow, but this is unusual.
                     }

                    // Mask is usually 1-bit per pixel, height might be doubled if XOR mask present (not handled here simply)
                    // We only need the basic AND mask part typically stored with hbmMask

                    // Create a compatible DC and select the mask bitmap to query its bits
                    HDC hdcMem = CreateCompatibleDC(hdcScreen);
                    if (!hdcMem) {
                        std::cerr << "[IconUtils] CreateCompatibleDC failed." << std::endl;
                        ReleaseDC(NULL, hdcScreen);
                        DestroyIcon(hIcon);
                        return std::nullopt;
                    }
                    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconInfo.hbmMask);

                    BITMAPINFO bmiMask = {0};
                    bmiMask.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmiMask.bmiHeader.biWidth = iconWidth; // Use color width
                    bmiMask.bmiHeader.biHeight = -iconHeight; // Top-down DIB, use color height
                    bmiMask.bmiHeader.biPlanes = 1;
                    bmiMask.bmiHeader.biBitCount = 1; // Request 1-bit mask data
                    bmiMask.bmiHeader.biCompression = BI_RGB;

                    // Query size needed for mask bits based on COLOR dimensions
                    GetDIBits(hdcMem, (HBITMAP)iconInfo.hbmMask, 0, iconHeight, NULL, &bmiMask, DIB_RGB_COLORS);
                    DWORD maskSize = bmiMask.bmiHeader.biSizeImage;
                     std::cout << "[IconUtils] Calculated mask DIB size: " << maskSize << " bytes." << std::endl;

                    if(maskSize > 0) {
                        maskPixels.resize(maskSize);
                        if (GetDIBits(hdcMem, (HBITMAP)iconInfo.hbmMask, 0, iconHeight, maskPixels.data(), &bmiMask, DIB_RGB_COLORS) > 0) {
                            std::cout << "[IconUtils] Successfully got mask DIBits." << std::endl;
                            // Now combine colorPixels (BGRA) and maskPixels (1-bit) into final RGBA
                            pixels.resize(iconWidth * iconHeight * 4);
                            int maskBytesPerRow = (iconWidth + 31) / 32 * 4; // Bytes per row in the mask DIB, padded to 4 bytes

                            for (int y = 0; y < iconHeight; ++y) {
                                for (int x = 0; x < iconWidth; ++x) {
                                    int pixelIndex = (y * iconWidth + x);
                                    int rgbaIndex = pixelIndex * 4;
                                    int bgraIndex = rgbaIndex; // Assuming BGRA format from GetDIBits

                                    // Get the mask bit (masks are often padded per line)
                                    int maskByteIndex = (y * maskBytesPerRow) + (x / 8); // Calculate byte index in DIB buffer
                                    int maskBitIndex = 7 - (x % 8); // Bit index within the byte (MSB first)

                                    // Ensure we don't read past the buffer if maskSize was smaller than expected
                                    bool isTransparent = false;
                                    if (maskByteIndex < maskPixels.size()) {
                                         isTransparent = ((maskPixels[maskByteIndex] >> maskBitIndex) & 1);
                                    } else {
                                         // This shouldn't happen if maskSize was calculated correctly
                                         // std::cerr << "[IconUtils] Warning: Read past mask buffer at y=" << y << ", x=" << x << std::endl;
                                    }


                                    // Copy BGR, set Alpha based on mask
                                    pixels[rgbaIndex + 0] = colorPixels[bgraIndex + 2]; // Blue -> Red
                                    pixels[rgbaIndex + 1] = colorPixels[bgraIndex + 1]; // Green -> Green
                                    pixels[rgbaIndex + 2] = colorPixels[bgraIndex + 0]; // Red -> Blue
                                    pixels[rgbaIndex + 3] = isTransparent ? 0 : 255;  // Alpha (0 = transparent, 255 = opaque)
                                }
                            }
                            success = true;
                            std::cout << "[IconUtils] Successfully converted HICON to RGBA pixels (" << iconWidth << "x" << iconHeight << ")." << std::endl;
                        } else {
                             std::cerr << "[IconUtils] GetDIBits failed for mask bitmap. Error code: " << GetLastError() << std::endl;
                             success = false; // Ensure success is false if mask extraction failed
                        }
                    } else {
                         std::cerr << "[IconUtils] Calculated mask size is zero or GetDIBits query failed." << std::endl;
                         success = false; // Ensure success is false
                    }

                    SelectObject(hdcMem, hbmOld); // Deselect bitmap
                    DeleteDC(hdcMem);
                     std::cout << "[IconUtils] Memory DC for mask deleted." << std::endl;
                } else {
                     std::cerr << "[IconUtils] IconInfo missing mask bitmap (hbmMask), cannot determine transparency." << std::endl;
                      // Fallback: Assume opaque if no mask? Or treat as error? Let's treat as error for now.
                      success = false;
                     // Alternatively, create fully opaque image:
                     /*
                     pixels.resize(iconWidth * iconHeight * 4);
                     for (int y = 0; y < iconHeight; ++y) {
                         for (int x = 0; x < iconWidth; ++x) {
                             int rgbaIndex = (y * iconWidth + x) * 4;
                             pixels[rgbaIndex + 0] = colorPixels[rgbaIndex + 2]; // B -> R
                             pixels[rgbaIndex + 1] = colorPixels[rgbaIndex + 1]; // G -> G
                             pixels[rgbaIndex + 2] = colorPixels[rgbaIndex + 0]; // R -> B
                             pixels[rgbaIndex + 3] = 255;                        // Alpha = Opaque
                         }
                     }
                     success = true;
                     std::cout << "[IconUtils] Warning: No mask found, created opaque RGBA image." << std::endl;
                     */

                }
            } else { std::cerr << "[IconUtils] GetDIBits failed for color bitmap. Error code: " << GetLastError() << std::endl; }
            ReleaseDC(NULL, hdcScreen);
             std::cout << "[IconUtils] Screen DC released." << std::endl;
        } else { std::cerr << "[IconUtils] IconInfo missing color bitmap (hbmColor)." << std::endl; }

        // GDI objects hbmColor and hbmMask are cleaned up by unique_ptr guards
        // We still need to destroy the original HICON handle
    } else {
        std::cerr << "[IconUtils] GetIconInfo failed for the icon handle. Error code: " << GetLastError() << std::endl;
    }

    // --- Step 5: Save RGBA pixels to PNG using wide char file access ---
    std::optional<std::string> resultPath = std::nullopt;
#ifdef _WIN32
    if (success && !pixels.empty() && iconWidth > 0 && iconHeight > 0) {
        std::cout << "[IconUtils] Preparing to save PNG (Windows)..." << std::endl;
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

                int close_result = fclose(fp); // Close the file regardless of write result
                 if (close_result != 0) {
                    std::cerr << "[IconUtils] Warning: fclose failed after writing PNG. Error: " << strerror(errno) << std::endl;
                 }

                if (write_result) {
                    std::cout << "[IconUtils] Successfully saved icon using stbi_write_png_to_func to: " << outputPathUtf8 << std::endl;
                    resultPath = outputPathUtf8; // Return the original UTF-8 path
                } else {
                    std::cerr << "[IconUtils] Failed to write PNG data using stbi_write_png_to_func to: " << outputPathUtf8 << std::endl;
                    // Optionally delete the potentially incomplete file?
                     int remove_result = _wremove(wOutputPath.c_str());
                     if (remove_result != 0) {
                         std::cerr << "[IconUtils] Failed to remove potentially incomplete PNG file: " << outputPathUtf8 << ", Error: " << strerror(errno) << std::endl;
                     } else {
                          std::cout << "[IconUtils] Removed potentially incomplete PNG file: " << outputPathUtf8 << std::endl;
                     }
                }
            } else {
                char errBuffer[256];
                strerror_s(errBuffer, sizeof(errBuffer), err);
                std::cerr << "[IconUtils] Failed to open file for writing (wide) using _wfopen_s. Path: "
                          << outputPathUtf8 << ", Error code: " << err << " (" << errBuffer << ")" << std::endl;
            }
        }
    } else if (!success) {
         std::cerr << "[IconUtils] Skipping PNG save because HICON to RGBA conversion failed or produced no data." << std::endl;
    }
#else
    // Non-Windows implementation (assuming stbi_write_png handles UTF-8 paths correctly)
    if (success && !pixels.empty() && iconWidth > 0 && iconHeight > 0) {
        std::cout << "[IconUtils] Preparing to save PNG (Non-Windows)..." << std::endl;
        if (stbi_write_png(outputPathUtf8.c_str(), iconWidth, iconHeight, 4, pixels.data(), iconWidth * 4)) {
            std::cout << "[IconUtils] Successfully saved icon to: " << outputPathUtf8 << std::endl;
            resultPath = outputPathUtf8;
        } else {
            std::cerr << "[IconUtils] Failed to write PNG file using stb_image_write to: " << outputPathUtf8 << std::endl;
        }
    } else if (!success) {
        std::cerr << "[IconUtils] Skipping PNG save because HICON to RGBA conversion failed or produced no data." << std::endl;
    }
#endif

    // --- Step 6: Cleanup HICON ---
    if (hIcon) {
        DestroyIcon(hIcon);
         std::cout << "[IconUtils] Original HICON destroyed." << std::endl;
        hIcon = NULL;
    }

    // --- Step 7: Return Result ---
    if (resultPath.has_value()) {
        std::cout << "[IconUtils] Returning saved path: " << resultPath.value() << std::endl;
    } else {
         std::cout << "[IconUtils] Icon extraction/saving failed. Returning nullopt." << std::endl;
    }
    return resultPath;
}


#endif // _WIN32 