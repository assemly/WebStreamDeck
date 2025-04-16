 #include <iostream> // For error logging

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

#endif // _WIN32

// Add definitions for other string utility functions here if needed in the future.

} // namespace StringUtils
