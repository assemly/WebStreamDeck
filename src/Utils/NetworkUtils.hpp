#pragma once

#include <string>

namespace NetworkUtils {

    // Fetches the first suitable non-loopback IPv4 address.
    // Returns the IP address string, or an error message string if failed.
    std::string GetLocalIPv4();

} // namespace NetworkUtils