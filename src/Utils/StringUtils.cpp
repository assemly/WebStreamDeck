#include <iostream> // For error logging
#include <algorithm> // For std::transform
#include <cctype>    // For towlower
#include <sstream>   // For std::wstringstream
#include <iomanip>   // For std::setfill and std::setw
#include <vector>    // Include vector if needed, e.g., for WideToUtf8 implementation if it uses it
#include <locale>    // Potentially needed for ToLowerW on non-windows
#include <codecvt>    // Potentially needed for conversions on non-windows

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

// <<< ADDED: Definition for WStringToHex >>>
std::wstring WStringToHex(const std::wstring& input) {
    std::wstringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (wchar_t wc : input) {
        // 输出每个宽字符的十六进制值
        ss << L" 0x" << std::setw(4) << static_cast<unsigned int>(wc);
    }
    return ss.str();
}

// Helper function to convert key name string (case-insensitive) to VK code
WORD StringToVkCode(const std::string& keyName) {
    std::string upperKeyName = keyName;
    std::transform(upperKeyName.begin(), upperKeyName.end(), upperKeyName.begin(), ::toupper);

    // Modifier Keys
    if (upperKeyName == "CTRL" || upperKeyName == "CONTROL") return VK_CONTROL;
    if (upperKeyName == "ALT") return VK_MENU; 
    if (upperKeyName == "SHIFT") return VK_SHIFT;
    if (upperKeyName == "WIN" || upperKeyName == "WINDOWS" || upperKeyName == "LWIN") return VK_LWIN; 
    if (upperKeyName == "RWIN") return VK_RWIN; 

    // Function Keys
    if (upperKeyName == "F1") return VK_F1; if (upperKeyName == "F2") return VK_F2;
    if (upperKeyName == "F3") return VK_F3; if (upperKeyName == "F4") return VK_F4;
    if (upperKeyName == "F5") return VK_F5; if (upperKeyName == "F6") return VK_F6;
    if (upperKeyName == "F7") return VK_F7; if (upperKeyName == "F8") return VK_F8;
    if (upperKeyName == "F9") return VK_F9; if (upperKeyName == "F10") return VK_F10;
    if (upperKeyName == "F11") return VK_F11; if (upperKeyName == "F12") return VK_F12;
    
    // Typing Keys (A-Z, 0-9)
    if (upperKeyName.length() == 1) {
        char c = upperKeyName[0];
        if (c >= 'A' && c <= 'Z') return c;
        if (c >= '0' && c <= '9') return c;
    }

    // Special Keys
    if (upperKeyName == "SPACE" || upperKeyName == " ") return VK_SPACE;
    if (upperKeyName == "ENTER" || upperKeyName == "RETURN") return VK_RETURN;
    if (upperKeyName == "TAB") return VK_TAB;
    if (upperKeyName == "ESC" || upperKeyName == "ESCAPE") return VK_ESCAPE;
    if (upperKeyName == "BACKSPACE") return VK_BACK;
    if (upperKeyName == "DELETE" || upperKeyName == "DEL") return VK_DELETE;
    if (upperKeyName == "INSERT" || upperKeyName == "INS") return VK_INSERT;
    if (upperKeyName == "HOME") return VK_HOME;
    if (upperKeyName == "END") return VK_END;
    if (upperKeyName == "PAGEUP" || upperKeyName == "PGUP") return VK_PRIOR; 
    if (upperKeyName == "PAGEDOWN" || upperKeyName == "PGDN") return VK_NEXT; 
    if (upperKeyName == "LEFT") return VK_LEFT;
    if (upperKeyName == "RIGHT") return VK_RIGHT;
    if (upperKeyName == "UP") return VK_UP;
    if (upperKeyName == "DOWN") return VK_DOWN;
    if (upperKeyName == "CAPSLOCK") return VK_CAPITAL;
    if (upperKeyName == "NUMLOCK") return VK_NUMLOCK;
    if (upperKeyName == "SCROLLLOCK") return VK_SCROLL;
    if (upperKeyName == "PRINTSCREEN" || upperKeyName == "PRTSC") return VK_SNAPSHOT;

    // Numpad Keys
    if (upperKeyName == "NUMPAD0") return VK_NUMPAD0; if (upperKeyName == "NUMPAD1") return VK_NUMPAD1;
    if (upperKeyName == "NUMPAD2") return VK_NUMPAD2; if (upperKeyName == "NUMPAD3") return VK_NUMPAD3;
    if (upperKeyName == "NUMPAD4") return VK_NUMPAD4; if (upperKeyName == "NUMPAD5") return VK_NUMPAD5;
    if (upperKeyName == "NUMPAD6") return VK_NUMPAD6; if (upperKeyName == "NUMPAD7") return VK_NUMPAD7;
    if (upperKeyName == "NUMPAD8") return VK_NUMPAD8; if (upperKeyName == "NUMPAD9") return VK_NUMPAD9;
    if (upperKeyName == "MULTIPLY" || upperKeyName == "NUMPAD*") return VK_MULTIPLY;
    if (upperKeyName == "ADD" || upperKeyName == "NUMPAD+") return VK_ADD;
    if (upperKeyName == "SEPARATOR") return VK_SEPARATOR; // Usually locale-specific
    if (upperKeyName == "SUBTRACT" || upperKeyName == "NUMPAD-") return VK_SUBTRACT;
    if (upperKeyName == "DECIMAL" || upperKeyName == "NUMPAD.") return VK_DECIMAL;
    if (upperKeyName == "DIVIDE" || upperKeyName == "NUMPAD/") return VK_DIVIDE;

    // OEM Keys (Common US Layout)
    if (upperKeyName == "+" || upperKeyName == "=") return VK_OEM_PLUS; 
    if (upperKeyName == "-" || upperKeyName == "_") return VK_OEM_MINUS;
    if (upperKeyName == "," || upperKeyName == "<") return VK_OEM_COMMA;
    if (upperKeyName == "." || upperKeyName == ">") return VK_OEM_PERIOD;
    if (upperKeyName == "/" || upperKeyName == "?") return VK_OEM_2; 
    if (upperKeyName == "`" || upperKeyName == "~") return VK_OEM_3; 
    if (upperKeyName == "[" || upperKeyName == "{") return VK_OEM_4; 
    if (upperKeyName == "\\" || upperKeyName == "|") return VK_OEM_5; 
    if (upperKeyName == "]" || upperKeyName == "}") return VK_OEM_6; 
    if (upperKeyName == "'" || upperKeyName == "\"") return VK_OEM_7; // Single/double quote
    if (upperKeyName == ";" || upperKeyName == ":") return VK_OEM_1; // Semicolon/colon

    std::cerr << "Warning: Unknown key name '" << keyName << "'" << std::endl;
    return 0; // Return 0 for unknown keys
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
