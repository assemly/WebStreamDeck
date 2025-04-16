#include <iostream> // For error logging
#include <algorithm> // For std::transform
#include <cctype>    // For towlower

namespace StringUtils {

#ifdef _WIN32
#include <windows.h> // For MultiByteToWideChar API

// Definition (Implementation) for the conversion function (Windows specific)
std::wstring Utf8ToWide(const std::string& utf8_str) {
    if (utf8_str.empty()) {
        return std::wstring();
    }
    // First, find the required buffer size
    int required_size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
    if (required_size == 0) {
        std::cerr << "Error: MultiByteToWideChar failed to get required size. Error code: " << GetLastError() << std::endl;
        return std::wstring();
    }
    // Allocate buffer and perform the conversion
    std::wstring wide_str(required_size, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], required_size);
    if (result == 0) {
        std::cerr << "Error: MultiByteToWideChar failed to convert string. Error code: " << GetLastError() << std::endl;
        return std::wstring();
    }
    // Remove the null terminator written by MultiByteToWideChar if size included it
    if (!wide_str.empty() && wide_str.back() == L'\0') {
         wide_str.pop_back();
    }
    return wide_str;
}

// <<< ADDED: Definition for WideToUtf8 >>>
std::string WideToUtf8(const std::wstring& wide_str) {
    if (wide_str.empty()) {
        return std::string();
    }
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_str[0], (int)wide_str.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) { 
         std::cerr << "[StringUtils] WideCharToMultiByte failed to calculate size. Error code: " << GetLastError() << std::endl;
         return ""; 
    }
    std::string utf8_str(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, &wide_str[0], (int)wide_str.size(), &utf8_str[0], size_needed, NULL, NULL);
     if (bytes_converted <= 0) { 
         std::cerr << "[StringUtils] WideCharToMultiByte failed to convert. Error code: " << GetLastError() << std::endl;
         return ""; 
    }
    return utf8_str;
}

// <<< ADDED: Definition for ToLowerW >>>
std::wstring ToLowerW(std::wstring str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](wchar_t c){ return towlower(c); });
    return str;
}

#else
// <<< ADDED: Non-Windows definitions/stubs if declared in header >>>
#include <algorithm> // For std::transform
#include <cwctype>   // For towlower

std::wstring ToLowerW(std::wstring str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](wchar_t c){ return towlower(c); });
    return str;
}

// Note: The stub WideToUtf8 defined inline in the header is likely sufficient for now
// if non-Windows doesn't actually use wstring paths.

#endif // _WIN32

// Add definitions for other string utility functions here if needed in the future.

} // namespace StringUtils
