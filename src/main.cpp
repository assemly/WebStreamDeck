// Keep only necessary includes
#include <iostream> // For std::cerr in main
#include <vector>   // For std::vector (global definition)
#include <string>   // For std::string/wstring (global definition)

#ifdef _WIN32
// Keep windows.h for the #ifdef block structure, maybe needed types like UINT if used implicitly
#define WIN32_LEAN_AND_MEAN // Keep lean
#include <windows.h>
#endif

// Include the new Application header
#include "Application.hpp"

// --- Global Mutex Handle (Windows specific) ---
#ifdef _WIN32
HANDLE g_hMutex = NULL;
const wchar_t* MUTEX_NAME = L"Global\\WebStreamDeckAppMutex_eb1d81a4-e271-4f22-87f7-3b667d4a3cdd"; // Use Global\ prefix for system-wide visibility
#endif

// --- Simplified Main Function ---
// int main(int argc, char** argv)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _WIN32
    // --- Single Instance Check ---
    g_hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME); // Attempt to own the mutex immediately
    if (g_hMutex == NULL) {
        // Failed to create mutex for some reason other than it already existing.
        MessageBoxW(NULL, L"Failed to create mutex. Application cannot start.", L"WebStreamDeck Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE; // Or handle error appropriately
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Mutex already exists, meaning another instance is running.
        CloseHandle(g_hMutex); // Close the handle we obtained

        // Optional: Bring the existing window to the foreground
        HWND hWnd = FindWindowW(L"GLFW30", L"WebStreamDeck"); // Replace with actual class/title
        if (hWnd) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else {
             MessageBoxW(NULL, L"WebStreamDeck is already running.", L"WebStreamDeck", MB_OK | MB_ICONINFORMATION);
         }

        return EXIT_FAILURE; // Exit this instance
    }
    // If we reached here, this is the first instance and we own the mutex.
#endif

    // Exception handling can be added here if Application constructor throws
    int exitCode = EXIT_SUCCESS; // Use a variable to store exit code
    try {
        Application app;
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception in main: " << e.what() << std::endl;
        // Consider showing a message box here as well for GUI apps
        MessageBoxW(NULL, L"An unhandled exception occurred. See console for details.", L"WebStreamDeck Error", MB_OK | MB_ICONERROR);
        exitCode = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown unhandled exception in main." << std::endl;
        // Consider showing a message box here as well for GUI apps
        MessageBoxW(NULL, L"An unknown unhandled exception occurred. See console for details.", L"WebStreamDeck Error", MB_OK | MB_ICONERROR);
        exitCode = EXIT_FAILURE;
    }

#ifdef _WIN32
    // --- Release Mutex ---
    if (g_hMutex) {
        ReleaseMutex(g_hMutex); // Release ownership
        CloseHandle(g_hMutex);  // Close the handle
        g_hMutex = NULL;
    }
#endif

    return exitCode; // Return the stored exit code
} 