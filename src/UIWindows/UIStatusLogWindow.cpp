#include "UIStatusLogWindow.hpp"
#include <iostream> // For std::cout, std::cerr

UIStatusLogWindow::UIStatusLogWindow(TranslationManager& translationManager)
    : m_translator(translationManager) {
    // Initialize language index based on current translator language
    const auto& availableLangs = m_translator.getAvailableLanguages();
    const std::string& currentLang = m_translator.getCurrentLanguage();
    m_currentLangIndex = -1;
    for (size_t i = 0; i < availableLangs.size(); ++i) {
        if (availableLangs[i] == currentLang) {
            m_currentLangIndex = static_cast<int>(i);
            break;
        }
    }
     if (m_currentLangIndex == -1 && !availableLangs.empty()) {
         m_currentLangIndex = 0; // Default to first language if current not found
     }
}


void UIStatusLogWindow::Draw(bool isServerRunning, int serverPort, const std::string& serverIP, std::function<void()> refreshIpCallback) {
    ImGui::Begin(m_translator.get("status_log_window_title").c_str());

    // --- Server Status Display ---
    ImGui::Text("%s %s", m_translator.get("server_status_label").c_str(),
              isServerRunning ? m_translator.get("server_status_running").c_str()
                                : m_translator.get("server_status_stopped").c_str());

    // Check if IP is valid before displaying addresses
    bool isIpValid = (serverIP.find("Error") == std::string::npos &&
                      serverIP.find("Fetching") == std::string::npos &&
                      serverIP.find("No suitable") == std::string::npos &&
                      !serverIP.empty());

    if (isServerRunning && serverPort > 0 && isIpValid) {
         ImGui::Text("%s http://%s:%d", m_translator.get("web_ui_address_label").c_str(), serverIP.c_str(), serverPort);
         ImGui::Text("%s ws://%s:%d", m_translator.get("websocket_address_label").c_str(), serverIP.c_str(), serverPort);
    } else {
        ImGui::Text("%s %s", m_translator.get("server_address_label").c_str(),
                  m_translator.get("server_address_error").c_str());
    }

    // Refresh IP Button - Use the callback provided by UIManager
    if (ImGui::Button(m_translator.get("refresh_ip_button").c_str())) {
        if (refreshIpCallback) {
            refreshIpCallback();
        }
    }
    ImGui::Separator();

    // --- Logs Section ---
    ImGui::TextUnformatted(m_translator.get("logs_header").c_str());
    // TODO: Implement actual logging display here (needs access to a log buffer/system)
    ImGui::TextWrapped("Log display area placeholder..."); // Placeholder
    ImGui::Separator();

    // --- Language Selection ---
    const auto& availableLangs = m_translator.getAvailableLanguages();
    std::vector<const char*> langItems;
    langItems.reserve(availableLangs.size()); // Pre-allocate memory
    for (const auto& lang : availableLangs) {
        langItems.push_back(lang.c_str());
    }

    // Ensure index is valid before using it
    if (m_currentLangIndex < 0 || m_currentLangIndex >= static_cast<int>(availableLangs.size())) {
       if (!availableLangs.empty()) m_currentLangIndex = 0; // Default to first if out of bounds
       else m_currentLangIndex = -1; // No languages available
    }


    ImGui::TextUnformatted("Language / 语言:");
    ImGui::SameLine();
    ImGui::PushItemWidth(150); // Give combo box a reasonable width
    if (ImGui::Combo("##LangCombo", &m_currentLangIndex, langItems.data(), langItems.size())) {
        if (m_currentLangIndex >= 0 && m_currentLangIndex < static_cast<int>(availableLangs.size())) {
            const std::string& selectedLang = availableLangs[m_currentLangIndex];
            if (m_translator.setLanguage(selectedLang)) {
                std::cout << "Language changed to: " << selectedLang << std::endl;
                // Potentially trigger font rebuild if necessary (though often handled elsewhere)
            } else {
                 std::cerr << "Failed to set language to: " << selectedLang << std::endl;
                 // Revert index? Or maybe translator handles this internally?
                 // For now, just log the error. The index might be out of sync if setLanguage fails.
                 // Re-fetch the index after attempt might be safer:
                 const std::string& actualLang = m_translator.getCurrentLanguage();
                 for(size_t i = 0; i < availableLangs.size(); ++i) {
                     if (availableLangs[i] == actualLang) {
                         m_currentLangIndex = static_cast<int>(i);
                         break;
                     }
                 }
            }
        }
    }
    ImGui::PopItemWidth();

    ImGui::End();
}