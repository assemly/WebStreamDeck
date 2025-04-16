#include <string>

namespace StringUtils {

#ifdef _WIN32
#include <windows.h> // For std::wstring definition on Windows even if API isn't used directly in header

// Declaration for the conversion function (Windows specific)
std::wstring Utf8ToWide(const std::string& utf8_str);

#else
// On other platforms, std::wstring might exist but the conversion isn't needed
// or needs a different approach. We can provide a stub declaration if needed,
// but often it's better to use #ifdefs where the function is called.
// For now, no declaration needed for non-Windows based on current usage.
#endif

} // namespace StringUtils 