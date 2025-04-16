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

// --- New Function Implementation ---

std::optional<ImageData> IconUtils::ConvertHIconToRGBA(HICON hIcon) {
    if (!hIcon) {
        std::cerr << "[IconUtils] ConvertHIconToRGBA: Received null HICON." << std::endl;
        return std::nullopt;
    }

    ICONINFO iconInfo = {0};
    bool success = false;
    ImageData resultData;

    std::cout << "[IconUtils] ConvertHIconToRGBA: Getting Icon Info..." << std::endl;
    if (GetIconInfo(hIcon, &iconInfo)) {
        // Wrap GDI objects for automatic cleanup using unique_ptr
        struct GdiObjectDeleter { void operator()(HGDIOBJ obj) const { if (obj) DeleteObject(obj); } };
        std::unique_ptr<void, GdiObjectDeleter> hbmColorGuard(iconInfo.hbmColor);
        std::unique_ptr<void, GdiObjectDeleter> hbmMaskGuard(iconInfo.hbmMask);

        if (iconInfo.hbmColor) {
            BITMAP bmpColor = {0};
            GetObject(iconInfo.hbmColor, sizeof(bmpColor), &bmpColor);
            resultData.width = bmpColor.bmWidth;
            resultData.height = bmpColor.bmHeight;
            std::cout << "[IconUtils] ConvertHIconToRGBA: Icon dimensions: " << resultData.width << "x" << resultData.height << std::endl;

            if (resultData.width <= 0 || resultData.height <= 0) {
                 std::cerr << "[IconUtils] ConvertHIconToRGBA: Invalid icon dimensions." << std::endl;
                 return std::nullopt; // Invalid dimensions
            }

            std::vector<unsigned char> colorPixels(resultData.width * resultData.height * 4);
            HDC hdcScreen = GetDC(NULL);
            if (!hdcScreen) {
                 std::cerr << "[IconUtils] ConvertHIconToRGBA: GetDC(NULL) failed." << std::endl;
                 return std::nullopt;
            }

            BITMAPINFO bmiColor = {0};
            bmiColor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmiColor.bmiHeader.biWidth = resultData.width;
            bmiColor.bmiHeader.biHeight = -resultData.height; // Request top-down DIB
            bmiColor.bmiHeader.biPlanes = 1;
            bmiColor.bmiHeader.biBitCount = 32; // Request 32-bit pixels (BGRA)
            bmiColor.bmiHeader.biCompression = BI_RGB;

            std::cout << "[IconUtils] ConvertHIconToRGBA: Getting color DIBits..." << std::endl;
            if (GetDIBits(hdcScreen, (HBITMAP)iconInfo.hbmColor, 0, resultData.height, colorPixels.data(), &bmiColor, DIB_RGB_COLORS) > 0) {
                std::cout << "[IconUtils] ConvertHIconToRGBA: Successfully got color DIBits." << std::endl;

                if (iconInfo.hbmMask) {
                    std::cout << "[IconUtils] ConvertHIconToRGBA: Mask bitmap (hbmMask) exists. Getting mask DIBits..." << std::endl;
                    BITMAP bmpMask = {0};
                    GetObject(iconInfo.hbmMask, sizeof(bmpMask), &bmpMask);

                    HDC hdcMem = CreateCompatibleDC(hdcScreen);
                    if (!hdcMem) {
                        std::cerr << "[IconUtils] ConvertHIconToRGBA: CreateCompatibleDC failed." << std::endl;
                        ReleaseDC(NULL, hdcScreen);
                        return std::nullopt;
                    }
                    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconInfo.hbmMask);

                    BITMAPINFO bmiMask = {0};
                    bmiMask.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmiMask.bmiHeader.biWidth = resultData.width;
                    bmiMask.bmiHeader.biHeight = -resultData.height; // Top-down DIB
                    bmiMask.bmiHeader.biPlanes = 1;
                    bmiMask.bmiHeader.biBitCount = 1; // Request 1-bit mask data
                    bmiMask.bmiHeader.biCompression = BI_RGB;

                    // Query size needed for mask bits
                    GetDIBits(hdcMem, (HBITMAP)iconInfo.hbmMask, 0, resultData.height, NULL, &bmiMask, DIB_RGB_COLORS);
                    DWORD maskSize = bmiMask.bmiHeader.biSizeImage;
                    std::cout << "[IconUtils] ConvertHIconToRGBA: Calculated mask DIB size: " << maskSize << " bytes." << std::endl;

                    if(maskSize > 0) {
                        std::vector<unsigned char> maskPixels(maskSize);
                        if (GetDIBits(hdcMem, (HBITMAP)iconInfo.hbmMask, 0, resultData.height, maskPixels.data(), &bmiMask, DIB_RGB_COLORS) > 0) {
                            std::cout << "[IconUtils] ConvertHIconToRGBA: Successfully got mask DIBits." << std::endl;
                            // Combine color and mask into final RGBA
                            resultData.pixels.resize(resultData.width * resultData.height * 4);
                            int maskBytesPerRow = (resultData.width + 31) / 32 * 4;

                            for (int y = 0; y < resultData.height; ++y) {
                                for (int x = 0; x < resultData.width; ++x) {
                                    int rgbaIndex = (y * resultData.width + x) * 4;
                                    int bgraIndex = rgbaIndex;
                                    int maskByteIndex = (y * maskBytesPerRow) + (x / 8);
                                    int maskBitIndex = 7 - (x % 8);

                                    bool isTransparent = false;
                                    if (maskByteIndex < maskPixels.size()) {
                                         isTransparent = ((maskPixels[maskByteIndex] >> maskBitIndex) & 1);
                                    }

                                    resultData.pixels[rgbaIndex + 0] = colorPixels[bgraIndex + 2]; // B -> R
                                    resultData.pixels[rgbaIndex + 1] = colorPixels[bgraIndex + 1]; // G -> G
                                    resultData.pixels[rgbaIndex + 2] = colorPixels[bgraIndex + 0]; // R -> B
                                    resultData.pixels[rgbaIndex + 3] = isTransparent ? 0 : 255;  // Alpha
                                }
                            }
                            success = true;
                            std::cout << "[IconUtils] ConvertHIconToRGBA: Successfully combined color and mask." << std::endl;
                        } else {
                             std::cerr << "[IconUtils] ConvertHIconToRGBA: GetDIBits failed for mask bitmap. Error: " << GetLastError() << std::endl;
                        }
                    } else {
                         std::cerr << "[IconUtils] ConvertHIconToRGBA: Calculated mask size is zero or GetDIBits query failed." << std::endl;
                    }
                    SelectObject(hdcMem, hbmOld);
                    DeleteDC(hdcMem);
                } else {
                     std::cerr << "[IconUtils] ConvertHIconToRGBA: IconInfo missing mask bitmap (hbmMask). Creating opaque image." << std::endl;
                     // Fallback: Create fully opaque image if no mask exists
                     resultData.pixels.resize(resultData.width * resultData.height * 4);
                     for (int y = 0; y < resultData.height; ++y) {
                         for (int x = 0; x < resultData.width; ++x) {
                             int rgbaIndex = (y * resultData.width + x) * 4;
                             resultData.pixels[rgbaIndex + 0] = colorPixels[rgbaIndex + 2]; // B -> R
                             resultData.pixels[rgbaIndex + 1] = colorPixels[rgbaIndex + 1]; // G -> G
                             resultData.pixels[rgbaIndex + 2] = colorPixels[rgbaIndex + 0]; // R -> B
                             resultData.pixels[rgbaIndex + 3] = 255;                        // Alpha = Opaque
                         }
                     }
                     success = true;
                }
            } else {
                 std::cerr << "[IconUtils] ConvertHIconToRGBA: GetDIBits failed for color bitmap. Error: " << GetLastError() << std::endl;
            }
            ReleaseDC(NULL, hdcScreen);
        } else {
             std::cerr << "[IconUtils] ConvertHIconToRGBA: IconInfo missing color bitmap (hbmColor)." << std::endl;
        }
        // GDI objects hbmColor and hbmMask automatically cleaned up by unique_ptr guards
    } else {
        std::cerr << "[IconUtils] ConvertHIconToRGBA: GetIconInfo failed. Error: " << GetLastError() << std::endl;
    }

    if (success) {
         std::cout << "[IconUtils] ConvertHIconToRGBA: Conversion successful." << std::endl;
        return resultData;
    } else {
         std::cout << "[IconUtils] ConvertHIconToRGBA: Conversion failed." << std::endl;
        return std::nullopt;
    }
}

// --- Modified Existing Function --- 

std::optional<std::string> IconUtils::ExtractAndSaveIconPng(
    const std::wstring& filePath,
    const std::string& outputDir,
    const std::string& desiredBaseName)
{
    std::cout << "[IconUtils] ExtractAndSaveIconPng: Attempting icon extraction for '" << desiredBaseName << "' from: "
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

    // 2. Construct Full Output Path
    fs::path fullPath = fs::path(outputDir) / (desiredBaseName + ".png");
    std::string outputPathUtf8 = fullPath.generic_string();
    std::cout << "[IconUtils] ExtractAndSaveIconPng: Target output path: " << outputPathUtf8 << std::endl;

    HICON hIcon = NULL;

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
         std::cerr << "[IconUtils] ExtractAndSaveIconPng: Unsupported file type: " << StringUtils::WideToUtf8(filePath) << std::endl;
    }

    if (!hIcon) {
        std::cerr << "[IconUtils] ExtractAndSaveIconPng: Failed to obtain HICON." << std::endl;
        return std::nullopt;
    }
    std::cout << "[IconUtils] ExtractAndSaveIconPng: Successfully obtained HICON." << std::endl;

    // --- Step 4: Convert HICON to RGBA using the new function --- 
    std::cout << "[IconUtils] ExtractAndSaveIconPng: Converting HICON to RGBA..." << std::endl;
    auto imageDataOpt = ConvertHIconToRGBA(hIcon);

    // We MUST destroy the icon handle obtained in Step 3, regardless of conversion success
    DestroyIcon(hIcon);
    hIcon = NULL; 
    std::cout << "[IconUtils] ExtractAndSaveIconPng: Original HICON destroyed." << std::endl;

    // --- Step 5: Save RGBA pixels to PNG --- 
    std::optional<std::string> resultPath = std::nullopt;
#ifdef _WIN32
    if (imageDataOpt) {
        const auto& imgData = *imageDataOpt;
        std::cout << "[IconUtils] ExtractAndSaveIconPng: Preparing to save PNG..." << std::endl;
        std::wstring wOutputPath = StringUtils::Utf8ToWide(outputPathUtf8);
        if (wOutputPath.empty() && !outputPathUtf8.empty()) {
            std::cerr << "[IconUtils] Failed to convert output path to wstring: " << outputPathUtf8 << std::endl;
        } else {
            FILE* fp = nullptr;
            errno_t err = _wfopen_s(&fp, wOutputPath.c_str(), L"wb");
            if (err == 0 && fp != nullptr) {
                 std::cout << "[IconUtils] ExtractAndSaveIconPng: File opened for writing." << std::endl;
                 int write_result = stbi_write_png_to_func(stbi_write_callback, fp, imgData.width, imgData.height, 4, imgData.pixels.data(), imgData.width * 4);
                 int close_result = fclose(fp);
                 if (close_result != 0) {
                    std::cerr << "[IconUtils] Warning: fclose failed after writing PNG. Error: " << strerror(errno) << std::endl;
                 }
                 if (write_result) {
                    std::cout << "[IconUtils] ExtractAndSaveIconPng: Successfully saved PNG to: " << outputPathUtf8 << std::endl;
                    resultPath = outputPathUtf8;
                 } else {
                     std::cerr << "[IconUtils] ExtractAndSaveIconPng: Failed to write PNG data." << std::endl;
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
    } else {
         std::cerr << "[IconUtils] ExtractAndSaveIconPng: Skipping PNG save due to HICON conversion failure." << std::endl;
    }
#else
    // Non-Windows PNG saving (can likely be removed if IconUtils is Windows-only)
    if (imageDataOpt) {
        const auto& imgData = *imageDataOpt;
         std::cout << "[IconUtils] ExtractAndSaveIconPng: Preparing to save PNG (Non-Windows)..." << std::endl;
         if (stbi_write_png(outputPathUtf8.c_str(), imgData.width, imgData.height, 4, imgData.pixels.data(), imgData.width * 4)) {
             /* ... success log ... */
             resultPath = outputPathUtf8;
         } else {
             /* ... error log ... */
         }
    }
#endif

    // --- Step 6: Cleanup (HICON already destroyed) ---

    // --- Step 7: Return Result --- 
    if (resultPath.has_value()) {
        std::cout << "[IconUtils] ExtractAndSaveIconPng: Returning saved path: " << resultPath.value() << std::endl;
    } else {
         std::cout << "[IconUtils] ExtractAndSaveIconPng: Icon extraction/saving failed. Returning nullopt." << std::endl;
    }
    return resultPath;
}

#endif // _WIN32 