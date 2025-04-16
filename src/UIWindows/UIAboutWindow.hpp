#pragma once

#include <imgui.h>
#include <string>
#include "../Managers/TranslationManager.hpp" // Include TranslationManager

class UIAboutWindow {
public:
    // Constructor requires TranslationManager
    UIAboutWindow(TranslationManager& translationManager);

    // Main drawing function
    void Draw();

    // Function to control visibility
    void Open();
    void Close();
    bool IsOpen() const;

private:
    TranslationManager& m_translator; // Reference to the translation manager
    bool m_isOpen = true;           // Controls if the window should be drawn
}; 