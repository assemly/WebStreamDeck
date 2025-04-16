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


// --- Simplified Main Function ---
//int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main(int argc, char** argv)
{
    // Exception handling can be added here if Application constructor throws
    try {
        Application app;
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception in main: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown unhandled exception in main." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
} 