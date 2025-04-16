#include "UIAboutWindow.hpp"
#include <imgui.h>
#include <string> // Make sure string is included if used
#include <cstdlib> // For system() on Linux/macOS
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // For ShellExecuteW
#include <shellapi.h> // For ShellExecuteW
#endif

UIAboutWindow::UIAboutWindow(TranslationManager& translationManager)
    : m_translator(translationManager) {}

void UIAboutWindow::Open() {
    m_isOpen = true;
}

void UIAboutWindow::Close() {
    m_isOpen = false;
}

bool UIAboutWindow::IsOpen() const {
    return m_isOpen;
}

void UIAboutWindow::Draw() {
    if (!m_isOpen) {
        return; // Don't draw if closed
    }

    // 不需要关闭按钮
    if (ImGui::Begin(m_translator.get("about_window_title").c_str())) {
        
        // --- Window Content --- 
        ImGui::Text(m_translator.get("app_name").c_str()); // Get App Name from Translator
        ImGui::Separator();

        ImGui::TextWrapped(m_translator.get("about_description").c_str());
        ImGui::Separator();

        ImGui::Text("%s:", m_translator.get("about_version").c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "1.2.7"); // TODO: Get actual version
        ImGui::Separator();

        ImGui::Text("%s:", m_translator.get("about_developer").c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "卡耐基的反复手"); // TODO: Replace with actual developer info
        // --- ADDED: Testers Info ---
        ImGui::Text("%s:", m_translator.get("about_testers").c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),  "GzsJAY_Official, 跑调, 兮的二次方");
        // --- END: Testers Info ---
        // --- ADDED: Bilibili Link ---
        ImGui::Text("%s:", m_translator.get("about_bilibili_profile").c_str());
        ImGui::SameLine();
        const char* url = "https://space.bilibili.com/5324474";
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), url); // Use a blue color for the link
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); // Change cursor to hand on hover
        }
        if (ImGui::IsItemClicked()) {
            #ifdef _WIN32
                ShellExecuteW(NULL, L"open", L"https://space.bilibili.com/5324474", NULL, NULL, SW_SHOWNORMAL);
            #elif __linux__
                std::string command = std::string("xdg-open ") + url;
                system(command.c_str());
            #elif __APPLE__
                std::string command = std::string("open ") + url;
                system(command.c_str());
            #else
                std::cerr << "Error: Opening URL not supported on this platform." << std::endl;
            #endif
        }
        // --- END: Bilibili Link ---
        
        ImGui::Separator();

        ImGui::Text("%s:", m_translator.get("about_usage_title").c_str());
        ImGui::BulletText(m_translator.get("about_usage_1").c_str());
        ImGui::BulletText(m_translator.get("about_usage_2").c_str());
        ImGui::BulletText(m_translator.get("about_usage_3").c_str());
        // Add more usage points as needed

        // ImGui::Separator();
        // if (ImGui::Button(m_translator.get("close_button").c_str())) {
        //     m_isOpen = false; // Close the window when the button is clicked
        // }
        
        // --- End Window Content ---
    }
    ImGui::End(); // Always call End() even if Begin() returned false
} 