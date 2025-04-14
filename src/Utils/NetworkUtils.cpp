#include "NetworkUtils.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream> // For std::cerr
#include <vector>   // For std::vector used internally
#include <memory>   // For smart pointers if needed, though raw pointers are used by API

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace NetworkUtils {

std::string GetLocalIPv4() {
    ULONG bufferSize = 15000; // Start with a suggested buffer size
    std::vector<BYTE> buffer(bufferSize); // Use a vector for automatic memory management
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // We want IPv4

    // Make an initial call to GetAdaptersAddresses to get the necessary buffer size
    DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        // Allocate the required buffer size
        buffer.resize(bufferSize);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        // Make a second call to GetAdaptersAddresses
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);
    }

    if (dwRetVal == NO_ERROR) {
        // Iterate through the list of adapters
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            // Check for operational adapters (Up), not loopback, and not tunnels
            if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
                pCurrAddresses->IfType != IF_TYPE_TUNNEL)
            {
                // Iterate through the unicast addresses for the current adapter
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast) {
                    sockaddr* pAddr = pUnicast->Address.lpSockaddr;
                    // Check if it's an IPv4 address
                    if (pAddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(pAddr);
                        // Convert the IP address to a string
                        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
                        return std::string(ipStr); // Found a suitable IP, return it
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
        // If loop completes without finding a suitable IP
        return "No suitable IP found";
    } else {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        return "Error fetching IP";
    }
    // Should not reach here if successful or error, but added for safety.
    return "Error fetching IP";
}

} // namespace NetworkUtils
