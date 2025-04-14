#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr
#include <qrcodegen.hpp> // Re-add QR Code generation library

#pragma comment(lib, "Ws2_32.lib") // Link against Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib") // Link against Iphlpapi.lib

// Helper to convert QR Code data to an OpenGL texture
GLuint qrCodeToTextureHelper(const qrcodegen::QrCode& qr) {
    if (qr.getSize() == 0) return 0;
    int size = qr.getSize();
    int border = 1; // Add a small border
    int texture_size = size + 2 * border;
    std::vector<unsigned char> texture_data(texture_size * texture_size * 3); // RGB
    for (int y = 0; y < texture_size; ++y) {
        for (int x = 0; x < texture_size; ++x) {
            bool is_black = false;
            if (x >= border && x < size + border && y >= border && y < size + border) {
                is_black = qr.getModule(x - border, y - border);
            }
            unsigned char color = is_black ? 0 : 255;
            int index = (y * texture_size + x) * 3;
            texture_data[index + 0] = color;
            texture_data[index + 1] = color;
            texture_data[index + 2] = color;
        }
    }
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

UIManager::UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager)
    : m_configManager(configManager),
      m_actionExecutor(actionExecutor),
      m_translator(translationManager)
{
    updateLocalIP(); // Get IP on startup
}

UIManager::~UIManager()
{
    releaseQrTexture(); // Release texture in destructor
}

void UIManager::drawUI()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    drawButtonGridWindow();
    drawConfigurationWindow();
    drawStatusLogWindow();
    drawQrCodeWindow();
}

void UIManager::drawButtonGridWindow()
{
    // Use translated title
    ImGui::Begin(m_translator.get("button_grid_window_title").c_str());

    const std::vector<ButtonConfig>& buttons = m_configManager.getButtons();

    if (buttons.empty()) {
        // Use translated text (reusing key from config window)
        ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
    } else {
        const float button_size = 100.0f;
        const int buttons_per_row = 4; // You might want to make this dynamic based on window width later
        int button_count = 0;

        for (const auto& button : buttons)
        {
            ImGui::PushID(button.id.c_str()); // Use button ID for unique ImGui ID
            
            // Use button name as label, handle potential encoding issues if necessary
            if (ImGui::Button(button.name.c_str(), ImVec2(button_size, button_size)))
            {
                printf("Button '%s' (ID: %s) clicked! Action: %s(%s)\n", 
                       button.name.c_str(), button.id.c_str(), 
                       button.action_type.c_str(), button.action_param.c_str());
                // Trigger the actual action execution here
                m_actionExecutor.executeAction(button.id);
            }

            button_count++;
            if (button_count % buttons_per_row != 0)
            {
                ImGui::SameLine();
            }
            else
            {
                button_count = 0; // Reset for next row (or handle wrapping better)
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void UIManager::drawConfigurationWindow()
{
    // Use translated window title
    ImGui::Begin(m_translator.get("config_window_title").c_str());

    const auto& buttons = m_configManager.getButtons(); // Get button data

    if (buttons.empty()) {
        // Use translated text
        ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
    } else {
        // Use translated header
        ImGui::TextUnformatted(m_translator.get("loaded_buttons_header").c_str());
        if (ImGui::BeginTable("buttons_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) { 
            // Keep using hardcoded internal identifiers for columns setup
            ImGui::TableSetupColumn("ID"); 
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow(); // Header row itself doesn't need translation (uses SetupColumn names)

            for (const auto& button : buttons) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", button.id.c_str()); // ID is data, not UI text
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", button.name.c_str()); // Name is data, not UI text

                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(button.id.c_str()); 
                // Edit button logic
                if (ImGui::SmallButton(m_translator.get("edit_button_label").c_str())) {
                    // Fetch the full config for the button to edit
                    auto buttonToEditOpt = m_configManager.getButtonById(button.id);
                    if (buttonToEditOpt) {
                        const auto& btnCfg = *buttonToEditOpt;
                        // Populate the editing state variables
                        m_editingButtonId = btnCfg.id;
                        strncpy(m_editButtonName, btnCfg.name.c_str(), IM_ARRAYSIZE(m_editButtonName) - 1); m_editButtonName[IM_ARRAYSIZE(m_editButtonName) - 1] = 0;
                        strncpy(m_editButtonActionType, btnCfg.action_type.c_str(), IM_ARRAYSIZE(m_editButtonActionType) - 1); m_editButtonActionType[IM_ARRAYSIZE(m_editButtonActionType) - 1] = 0;
                        strncpy(m_editButtonActionParam, btnCfg.action_param.c_str(), IM_ARRAYSIZE(m_editButtonActionParam) - 1); m_editButtonActionParam[IM_ARRAYSIZE(m_editButtonActionParam) - 1] = 0;
                        // strncpy(m_editButtonIconPath, btnCfg.icon_path.c_str(), IM_ARRAYSIZE(m_editButtonIconPath) - 1); m_editButtonIconPath[IM_ARRAYSIZE(m_editButtonIconPath) - 1] = 0;
                        
                        m_showEditModal = true; // Set flag to show the modal
                    } else {
                         std::cerr << "Error: Could not find button data for ID: " << button.id << " to edit." << std::endl;
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(m_translator.get("delete_button_label").c_str())) {
                    m_buttonIdToDelete = button.id;
                    m_showDeleteConfirmation = true; 
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    
    ImGui::Separator();

    // --- Section to Add New Buttons ---
    // Use translated header
    ImGui::TextUnformatted(m_translator.get("add_new_button_header").c_str());
    
    // Use translated labels for input fields
    ImGui::InputText(m_translator.get("button_id_label").c_str(), m_newButtonId, IM_ARRAYSIZE(m_newButtonId));
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        // Use translated tooltip
        ImGui::TextUnformatted(m_translator.get("button_id_tooltip").c_str());
        ImGui::EndTooltip();
    }

    ImGui::InputText(m_translator.get("button_name_label").c_str(), m_newButtonName, IM_ARRAYSIZE(m_newButtonName));
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        // Use translated tooltip
        ImGui::TextUnformatted(m_translator.get("button_name_tooltip").c_str());
        ImGui::EndTooltip();
    }

    ImGui::InputText(m_translator.get("action_type_label").c_str(), m_newButtonActionType, IM_ARRAYSIZE(m_newButtonActionType)); 
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        // Use translated tooltip
        ImGui::TextUnformatted(m_translator.get("action_type_tooltip").c_str());
        ImGui::EndTooltip();
    }

    ImGui::InputText(m_translator.get("action_param_label").c_str(), m_newButtonActionParam, IM_ARRAYSIZE(m_newButtonActionParam));
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        // Use translated tooltip
        ImGui::TextUnformatted(m_translator.get("action_param_tooltip").c_str());
        ImGui::EndTooltip();
    }

    // Use translated label for Add button
    if (ImGui::Button(m_translator.get("add_button_label").c_str())) {
        ButtonConfig newButton;
        newButton.id = m_newButtonId;
        newButton.name = m_newButtonName;
        newButton.action_type = m_newButtonActionType;
        newButton.action_param = m_newButtonActionParam;

        if (m_configManager.addButton(newButton)) {
            // Use translated log message (or keep logs in English? Decision needed)
            std::cout << m_translator.get("button_added_success_log") << newButton.id << std::endl;
            if (m_configManager.saveConfig()) {
                 std::cout << m_translator.get("config_saved_add_log") << std::endl;
                 m_newButtonId[0] = '\0';
                 m_newButtonName[0] = '\0';
                 m_newButtonActionType[0] = '\0';
                 m_newButtonActionParam[0] = '\0';
            } else {
                std::cerr << m_translator.get("config_save_fail_add_log") << std::endl;
            }
        } else {
            std::cerr << m_translator.get("add_button_fail_log") << std::endl;
        }
    }

    // --- Delete Confirmation Modal ---
    if (m_showDeleteConfirmation) {
        // Use translated title
        ImGui::OpenPopup(m_translator.get("delete_confirm_title").c_str());
        m_showDeleteConfirmation = false; 
    }

    // Use translated title for BeginPopupModal check
    if (ImGui::BeginPopupModal(m_translator.get("delete_confirm_title").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Use translated text
        ImGui::TextUnformatted(m_translator.get("delete_confirm_text").c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s", m_buttonIdToDelete.c_str()); 
        ImGui::Separator();

        // Use translated button labels
        if (ImGui::Button(m_translator.get("delete_confirm_yes").c_str(), ImVec2(120, 0))) { 
            if (m_configManager.removeButton(m_buttonIdToDelete)) {
                std::cout << m_translator.get("button_removed_log") << m_buttonIdToDelete << std::endl;
                if (m_configManager.saveConfig()) {
                    std::cout << m_translator.get("config_saved_delete_log") << std::endl;
                } else {
                    std::cerr << m_translator.get("config_save_fail_delete_log") << std::endl;
                }
            } else {
                 // Combine log parts
                 std::cerr << m_translator.get("remove_button_fail_log") << m_buttonIdToDelete 
                           << m_translator.get("remove_button_fail_log_suffix") << std::endl;
            }
            m_buttonIdToDelete = ""; 
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        // Use translated button label
        if (ImGui::Button(m_translator.get("delete_confirm_cancel").c_str(), ImVec2(120, 0))) { 
            std::cout << m_translator.get("delete_cancel_log") << m_buttonIdToDelete << std::endl;
            m_buttonIdToDelete = ""; 
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::EndPopup();
    }

    // --- Edit Button Modal --- 
    if (m_showEditModal) {
        ImGui::OpenPopup("Edit Button"); // Use a simple title for the popup ID
        m_showEditModal = false; // Reset flag after opening popup
    }

    // Define the modal window for editing
    if (ImGui::BeginPopupModal("Edit Button", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Editing Button ID: %s (Cannot be changed)", m_editingButtonId.c_str());
        ImGui::Separator();

        ImGui::InputText(m_translator.get("button_name_label").c_str(), m_editButtonName, IM_ARRAYSIZE(m_editButtonName));
        // TODO: Add tooltips like in the Add section
        ImGui::InputText(m_translator.get("action_type_label").c_str(), m_editButtonActionType, IM_ARRAYSIZE(m_editButtonActionType));
        ImGui::InputText(m_translator.get("action_param_label").c_str(), m_editButtonActionParam, IM_ARRAYSIZE(m_editButtonActionParam));
        // TODO: Input for icon path if needed

        ImGui::Separator();

        // Use translated labels
        if (ImGui::Button(m_translator.get("save_changes_button_label").c_str(), ImVec2(120, 0))) { // Need key "save_changes_button_label"
            ButtonConfig updatedButton;
            updatedButton.id = m_editingButtonId; // Keep original ID
            updatedButton.name = m_editButtonName;
            updatedButton.action_type = m_editButtonActionType;
            updatedButton.action_param = m_editButtonActionParam;
            // updatedButton.icon_path = m_editButtonIconPath;

            if (m_configManager.updateButton(m_editingButtonId, updatedButton)) {
                std::cout << "Button updated successfully: " << m_editingButtonId << std::endl;
                if (m_configManager.saveConfig()) {
                    std::cout << "Configuration saved after updating button." << std::endl;
                } else {
                    std::cerr << "Error: Failed to save configuration after updating button." << std::endl;
                }
            } else {
                 std::cerr << "Error: Failed to update button " << m_editingButtonId << " (check console)." << std::endl;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        // Use translated labels
        if (ImGui::Button(m_translator.get("cancel_button_label").c_str(), ImVec2(120, 0))) { // Need key "cancel_button_label"
            std::cout << "Edit cancelled for button ID: " << m_editingButtonId << std::endl;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void UIManager::drawStatusLogWindow()
{
    // Use translated title
    ImGui::Begin(m_translator.get("status_log_window_title").c_str());
    
    // Use translated label and status texts
    ImGui::Text("%s %s", m_translator.get("server_status_label").c_str(), 
              m_isServerRunning ? m_translator.get("server_status_running").c_str() 
                                : m_translator.get("server_status_stopped").c_str());
    
    if (m_isServerRunning && m_serverPort > 0 && m_serverIP.find("Error") == std::string::npos && m_serverIP.find("Fetching") == std::string::npos && m_serverIP.find("No suitable") == std::string::npos) {
         // Use translated labels
         ImGui::Text("%s http://%s:%d", m_translator.get("web_ui_address_label").c_str(), m_serverIP.c_str(), m_serverPort);
         ImGui::Text("%s ws://%s:%d", m_translator.get("websocket_address_label").c_str(), m_serverIP.c_str(), m_serverPort);
    } else {
        // Use translated labels/messages (requires "server_address_label" key in JSON)
        ImGui::Text("%s %s", m_translator.get("server_address_label").c_str(), 
                  m_translator.get("server_address_error").c_str());
    }
    // Use translated button label
    if (ImGui::Button(m_translator.get("refresh_ip_button").c_str())) {
        updateLocalIP();
    }
    ImGui::Separator();
    // Use translated header
    ImGui::TextUnformatted(m_translator.get("logs_header").c_str());
    // TODO: Implement actual logging display here

    ImGui::Separator(); // Separator before language selection

    // --- Language Selection --- 
    // Get available languages and current language index
    const auto& availableLangs = m_translator.getAvailableLanguages();
    const std::string& currentLang = m_translator.getCurrentLanguage();
    int currentLangIndex = -1;
    std::vector<const char*> langItems; // Need C-style strings for ImGui::Combo
    for (size_t i = 0; i < availableLangs.size(); ++i) {
        langItems.push_back(availableLangs[i].c_str());
        if (availableLangs[i] == currentLang) {
            currentLangIndex = static_cast<int>(i);
        }
    }

    ImGui::TextUnformatted("Language / 语言:"); // Simple label
    ImGui::SameLine();
    // Combo box to select language
    if (ImGui::Combo("##LangCombo", &currentLangIndex, langItems.data(), langItems.size())) {
        if (currentLangIndex >= 0 && currentLangIndex < availableLangs.size()) {
            const std::string& selectedLang = availableLangs[currentLangIndex];
            if (m_translator.setLanguage(selectedLang)) {
                std::cout << "Language changed to: " << selectedLang << std::endl;
                // Font might need to be rebuilt or reloaded if glyph ranges differ significantly
                // between languages, but Noto Sans SC should cover both en/zh well.
            } else {
                 std::cerr << "Failed to set language to: " << selectedLang << std::endl;
            }
        }
    }

    ImGui::End();
}

void UIManager::drawQrCodeWindow()
{
    // Use translated title
    ImGui::Begin(m_translator.get("qr_code_window_title").c_str());
    if (m_isServerRunning && m_serverPort > 0 && m_serverIP.find("Error") == std::string::npos && m_serverIP.find("Fetching") == std::string::npos && m_serverIP.find("No suitable") == std::string::npos) {
        std::string http_address = "http://" + m_serverIP + ":" + std::to_string(m_serverPort);

        if (http_address != m_lastGeneratedQrText) {
            generateQrTexture(http_address); 
        }
        
        // Use translated text
        ImGui::TextUnformatted(m_translator.get("scan_qr_code_prompt_1").c_str());
        ImGui::TextUnformatted(m_translator.get("scan_qr_code_prompt_2").c_str());
        if (m_qrTextureId != 0) {
            ImGui::Image((ImTextureID)(intptr_t)m_qrTextureId, ImVec2(200, 200));
        } else {
            // Use translated error message
            ImGui::TextUnformatted(m_translator.get("qr_code_failed").c_str());
        }
        // Use translated label
         ImGui::Text("%s %s", m_translator.get("open_in_browser_label").c_str(), http_address.c_str());
         // Use translated button label
         if (ImGui::Button(m_translator.get("copy_web_address_button").c_str())) {
             ImGui::SetClipboardText(http_address.c_str());
         }

    } else if (!m_isServerRunning) {
        // Use translated text
        ImGui::TextUnformatted(m_translator.get("server_stopped_qr_prompt").c_str());
         if (m_qrTextureId != 0) { releaseQrTexture(); } 
    } else {
        // Use translated text
        ImGui::TextUnformatted(m_translator.get("waiting_for_ip_qr_prompt").c_str());
        // Use translated button label
        if (ImGui::Button(m_translator.get("retry_fetch_ip_button").c_str())) {
            updateLocalIP();
        }
         if (m_qrTextureId != 0) { releaseQrTexture(); } 
    }
    ImGui::End();
}

// Helper to generate/update QR Code texture
void UIManager::generateQrTexture(const std::string& text) {
    releaseQrTexture(); // Release existing texture first
    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        m_qrTextureId = qrCodeToTextureHelper(qr); // Use the helper
        if (m_qrTextureId != 0) {
            m_lastGeneratedQrText = text; // Store the text used for this texture
        }
         else {
             m_lastGeneratedQrText = ""; // Generation failed
         }
    } catch (const std::exception& e) {
        std::cerr << "Error generating QR Code: " << e.what() << std::endl;
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Helper to release the QR Code texture
void UIManager::releaseQrTexture() {
    if (m_qrTextureId != 0) {
        glDeleteTextures(1, &m_qrTextureId);
        m_qrTextureId = 0;
        m_lastGeneratedQrText = "";
    }
}

// Implementation of the new method
void UIManager::setServerStatus(bool isRunning, int port) {
    m_isServerRunning = isRunning;
    m_serverPort = port;
}

// Helper function to get the local IPv4 address (Windows specific)
void UIManager::updateLocalIP() {
    std::string oldIP = m_serverIP;
    m_serverIP = "Error fetching IP"; // Default error message
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // We want IPv4

    // Try to get the buffer size needed
    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (!pAddresses) {
        std::cerr << "Memory allocation failed for GetAdaptersAddresses\n";
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        if (!pAddresses) {
            std::cerr << "Memory allocation failed for GetAdaptersAddresses\n";
            return;
        }
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);
    }

    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            // Look for operational adapters (Up) that are not loopback and support IPv4
            if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
                pCurrAddresses->IfType != IF_TYPE_TUNNEL) // Exclude tunnels like VPNs
            {
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast) {
                    sockaddr* pAddr = pUnicast->Address.lpSockaddr;
                    if (pAddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(pAddr);
                        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
                        m_serverIP = ipStr; // Found a suitable IP
                        if (oldIP != m_serverIP) { // Regenerate QR if IP changed
                           releaseQrTexture(); // Force regeneration in drawQrCodeWindow
                        }
                        free(pAddresses);
                        return; // Use the first suitable IPv4 address found
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
        m_serverIP = "No suitable IP found";
    } else {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
    }

    if (pAddresses) {
        free(pAddresses);
    }
    if (oldIP != m_serverIP) { // Also release texture if IP becomes invalid
        releaseQrTexture();
    }
} 