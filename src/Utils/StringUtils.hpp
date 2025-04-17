#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace StringUtils {

#ifdef _WIN32
#include <windows.h> // For std::wstring definition on Windows even if API isn't used directly in header

// Declaration for the conversion function (Windows specific)
std::wstring Utf8ToWide(const std::string& utf8_str);

// Convert UTF-8 std::string to std::wstring
std::wstring Utf8ToWide(const std::string& utf8_str);

// Convert std::wstring to UTF-8 std::string
std::string WideToUtf8(const std::wstring& wide_str);

// Convert std::wstring to lowercase
std::wstring ToLowerW(std::wstring str);

// Convert std::wstring to a hex representation string (for debugging)
std::wstring WStringToHex(const std::wstring& input);

// Helper function to convert key name string (case-insensitive) to VK code
WORD StringToVkCode(const std::string& keyName);

#else
// On other platforms, std::wstring might exist but the conversion isn't needed
// or needs a different approach. We can provide a stub declaration if needed,
// but often it's better to use #ifdefs where the function is called.
// For now, no declaration needed for non-Windows based on current usage.

// Example: Simple stub for WideToUtf8 if non-Win uses std::string directly
inline std::string WideToUtf8(const std::wstring& wide_str) { 
    // This is likely wrong - real conversion needed if wstring used on non-Win
    return ""; 
}

// Example: Stub for ToLowerW using standard C++ locale features (more complex)
std::wstring ToLowerW(std::wstring str);

#endif

} // namespace StringUtils 