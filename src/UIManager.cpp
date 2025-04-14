#include <GL/glew.h>    // Include GLEW to define GL functions and types
#include "UIManager.hpp"
#include <stdio.h> // For printf used in button click
#include <vector> // For std::vector
#include <winsock2.h> // For Windows Sockets
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <iphlpapi.h> // For GetAdaptersAddresses
#include <iostream>   // For std::cerr
#include <qrcodegen.hpp> // Re-add QR Code generation library
#include <ImGuiFileDialog.h> // ADDED ImGuiFileDialog header

// Define STB_IMAGE_IMPLEMENTATION in *one* CPP file before including stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>       // Include stb_image header for image loading

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

// --- ADDED: Implementation for loading texture from file ---
GLuint UIManager::LoadTextureFromFile(const char* filename) {
    int width, height, channels;
    // Force 4 channels (RGBA) for consistency with OpenGL formats
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (data == nullptr) {
        std::cerr << "Error loading image: " << filename << " - " << stbi_failure_reason() << std::endl;
        return 0; // Return 0 indicates failure
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "Loaded texture: " << filename << " (ID: " << textureID << ")" << std::endl;
    return textureID;
}

// --- End of LoadTextureFromFile --- 

// --- ADDED: Implementation for releasing animated GIF textures ---
void UIManager::releaseAnimatedGifTextures() {
    for (auto const& [path, gifData] : m_animatedGifTextures) {
        if (gifData.loaded) {
            glDeleteTextures(gifData.frameTextureIds.size(), gifData.frameTextureIds.data());
             std::cout << "Deleted " << gifData.frameTextureIds.size() << " GIF textures for: " << path << std::endl;
        }
    }
    m_animatedGifTextures.clear();
}
// --- End of releaseAnimatedGifTextures --- 

UIManager::UIManager(ConfigManager& configManager, ActionExecutor& actionExecutor, TranslationManager& translationManager)
    : m_configManager(configManager),
      m_actionExecutor(actionExecutor),
      m_translator(translationManager)
{
    updateLocalIP(); // Get IP on startup
}

// --- MODIFIED: Destructor to include GIF texture cleanup --- 
UIManager::~UIManager()
{
    releaseQrTexture(); // Release QR code texture (already present)

    // Release button icon textures (static)
    for (auto const& [path, textureId] : m_buttonIconTextures) {
        if (textureId != 0) { // Only delete valid texture IDs
            glDeleteTextures(1, &textureId);
             std::cout << "Deleted static texture for: " << path << " (ID: " << textureId << ")" << std::endl;
        }
    }
    m_buttonIconTextures.clear(); // Clear the map

    releaseAnimatedGifTextures(); // ADDED: Release animated GIF textures
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
    double currentTime = ImGui::GetTime(); // Get current time for animation

    if (buttons.empty()) {
        // Use translated text (reusing key from config window)
        ImGui::TextUnformatted(m_translator.get("no_buttons_loaded").c_str());
    } else {
        const float button_size = 100.0f;
        const int buttons_per_row = 4; // You might want to make this dynamic based on window width later
        int button_index_in_row = 0; // Track position in the current row

        for (const auto& button : buttons)
        {
            ImGui::PushID(button.id.c_str()); // Use button ID for unique ImGui ID
            
            GLuint textureID = 0; // Final texture ID to display
            bool useImageButton = false;
            std::string lowerIconPath = button.icon_path;
            std::transform(lowerIconPath.begin(), lowerIconPath.end(), lowerIconPath.begin(), ::tolower); // Convert to lowercase for extension check

            // --- MODIFIED: Icon Loading Logic --- 
            if (!button.icon_path.empty()) {
                if (lowerIconPath.length() > 4 && lowerIconPath.substr(lowerIconPath.length() - 4) == ".gif") {
                    // --- Handle Animated GIF --- 
                    auto it = m_animatedGifTextures.find(button.icon_path);
                    if (it == m_animatedGifTextures.end()) {
                        // Not loaded yet, try to load
                        GifLoader::AnimatedGif gifData;
                        if (GifLoader::LoadAnimatedGifFromFile(button.icon_path.c_str(), gifData)) {
                            gifData.lastFrameTime = currentTime; // Initialize timing on load
                             m_animatedGifTextures[button.icon_path] = std::move(gifData); // Move loaded data into map
                             it = m_animatedGifTextures.find(button.icon_path); // Get iterator again
                        } else {
                            // Loading failed, cache the failure (empty struct with loaded=false)
                            m_animatedGifTextures[button.icon_path] = {}; 
                             std::cerr << "Failed to load GIF for button '" << button.id << "' from path: " << button.icon_path << std::endl;
                        }
                    }

                    if (it != m_animatedGifTextures.end() && it->second.loaded && !it->second.frameTextureIds.empty()) {
                        // GIF is loaded, update animation frame
                        GifLoader::AnimatedGif& gif = it->second;
                        double timeSinceLastFrame = currentTime - gif.lastFrameTime;
                        double frameDelaySeconds = static_cast<double>(gif.frameDelaysMs[gif.currentFrame]) / 1000.0;
                        
                        if (timeSinceLastFrame >= frameDelaySeconds) {
                            gif.currentFrame = (gif.currentFrame + 1) % gif.frameTextureIds.size();
                            gif.lastFrameTime = currentTime; // Reset timer for the new frame
                        }
                        textureID = gif.frameTextureIds[gif.currentFrame];
                        useImageButton = true;
                    }
                } else {
                    // --- Handle Static Image (using stb_image via LoadTextureFromFile) --- 
                    auto it = m_buttonIconTextures.find(button.icon_path);
                    if (it != m_buttonIconTextures.end()) {
                        textureID = it->second;
                        if (textureID != 0) useImageButton = true;
                    } else {
                        textureID = LoadTextureFromFile(button.icon_path.c_str());
                        m_buttonIconTextures[button.icon_path] = textureID;
                        if (textureID != 0) {
                            useImageButton = true;
                        } else {
                            std::cerr << "Failed to load static texture for button '" << button.id << "' from path: " << button.icon_path << std::endl;
                        }
                    }
                }
            }

            // --- Button Rendering --- 
            bool buttonClicked = false;
            if (useImageButton && textureID != 0) {
                 buttonClicked = ImGui::ImageButton(button.id.c_str(),
                                                    (ImTextureID)(intptr_t)textureID,
                                                    ImVec2(button_size, button_size),
                                                    ImVec2(0, 0),
                                                    ImVec2(1, 1),
                                                    ImVec4(0,0,0,0),
                                                    ImVec4(1,1,1,1));
                 if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", button.name.c_str());
                 }
            } else {
                // Fallback to text button
                buttonClicked = ImGui::Button(button.name.c_str(), ImVec2(button_size, button_size));
            }
            // --- END of Button Rendering ---
            
            // Handle button click
            if (buttonClicked)
            {
                printf("Button '%s' (ID: %s) clicked! Action: %s(%s)\n", 
                       button.name.c_str(), button.id.c_str(), 
                       button.action_type.c_str(), button.action_param.c_str());
                m_actionExecutor.requestAction(button.id);
            }

            // Handle layout (wrapping)
            button_index_in_row++;
            if (button_index_in_row < buttons_per_row) 
            {
                ImGui::SameLine();
            }
            else
            {
                button_index_in_row = 0; // Reset for the next row
            }
            ImGui::PopID();
        }
         // Ensure the next item starts on a new line if the last row wasn't full
         if (button_index_in_row != 0) {
             ImGui::NewLine();
        }
    }

    ImGui::End();
}

void UIManager::drawConfigurationWindow()
{
    // Use translated window title
    ImGui::Begin(m_translator.get("config_window_title").c_str());

    const auto& buttons = m_configManager.getButtons(); // Get button data

    // --- Loaded Buttons Table ---
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
                // --- MODIFIED: Edit Button Behavior ---
                if (ImGui::SmallButton(m_translator.get("edit_button_label").c_str())) {
                    // Fetch the full config for the button to edit
                    auto buttonToEditOpt = m_configManager.getButtonById(button.id);
                    if (buttonToEditOpt) {
                        const auto& btnCfg = *buttonToEditOpt;
                        // Set editing mode and load data into the "Add New" fields
                        m_editingButtonId = btnCfg.id; // Mark as editing this ID
                        // Load data into the m_newButton... variables
                        strncpy(m_newButtonId, btnCfg.id.c_str(), IM_ARRAYSIZE(m_newButtonId) - 1); m_newButtonId[IM_ARRAYSIZE(m_newButtonId) - 1] = 0;
                        strncpy(m_newButtonName, btnCfg.name.c_str(), IM_ARRAYSIZE(m_newButtonName) - 1); m_newButtonName[IM_ARRAYSIZE(m_newButtonName) - 1] = 0;
                        m_newButtonActionTypeIndex = 0; // Default to first if not found
                        for(size_t i = 0; i < m_supportedActionTypes.size(); ++i) {
                            if (m_supportedActionTypes[i] == btnCfg.action_type) {
                                m_newButtonActionTypeIndex = static_cast<int>(i);
                                break;
                            }
                        }
                        strncpy(m_newButtonActionParam, btnCfg.action_param.c_str(), IM_ARRAYSIZE(m_newButtonActionParam) - 1); m_newButtonActionParam[IM_ARRAYSIZE(m_newButtonActionParam) - 1] = 0;
                        strncpy(m_newButtonIconPath, btnCfg.icon_path.c_str(), IM_ARRAYSIZE(m_newButtonIconPath) - 1); m_newButtonIconPath[IM_ARRAYSIZE(m_newButtonIconPath) - 1] = 0;

                        std::cout << "Editing button: " << m_editingButtonId << std::endl; // Log edit start
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

    // --- MODIFIED: Section for Add/Edit Button ---
    bool isEditing = !m_editingButtonId.empty();
    // Use translated header or custom Edit header
    ImGui::TextUnformatted(isEditing ? m_translator.get("edit_button_header").c_str() : m_translator.get("add_new_button_header").c_str());

    // --- BEGIN TABLE LAYOUT --- 
    // Use TableSetupColumn for better width control
    if (ImGui::BeginTable("add_edit_form", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV )) {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, 120.0f); // Fixed width for labels
        ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);      // Stretchable inputs

        // --- Row 1: Button ID ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(m_translator.get("button_id_label").c_str());
        
        ImGui::TableSetColumnIndex(1);
        float helpIconWidth = ImGui::CalcTextSize("(?)").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
        float idInputWidth = ImGui::GetContentRegionAvail().x - helpIconWidth - (isEditing ? ImGui::CalcTextSize("(Cannot be changed)").x + ImGui::GetStyle().ItemSpacing.x : 0.0f);
        ImGui::PushItemWidth(idInputWidth > 0 ? idInputWidth : -FLT_MIN); 
        ImGuiInputTextFlags idFlags = isEditing ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
        ImGui::InputText("##ButtonID", m_newButtonId, IM_ARRAYSIZE(m_newButtonId), idFlags);
        ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_id_tooltip").c_str()); }
        if (isEditing) {
            ImGui::SameLine(); ImGui::TextDisabled("(Cannot be changed)");
        }

        // --- Row 2: Button Name ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(m_translator.get("button_name_label").c_str());

        ImGui::TableSetColumnIndex(1);
        float nameInputWidth = ImGui::GetContentRegionAvail().x - helpIconWidth;
        ImGui::PushItemWidth(nameInputWidth > 0 ? nameInputWidth : -FLT_MIN);
        ImGui::InputText("##ButtonName", m_newButtonName, IM_ARRAYSIZE(m_newButtonName));
        ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_name_tooltip").c_str()); }

        // --- Row 3: Action Type ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(m_translator.get("action_type_label").c_str());

        ImGui::TableSetColumnIndex(1);
        float comboWidth = ImGui::GetContentRegionAvail().x - helpIconWidth;
        ImGui::PushItemWidth(comboWidth > 0 ? comboWidth : -FLT_MIN);
        std::vector<const char*> actionTypeDisplayItems;
        for(const auto& type : m_supportedActionTypes) {
            std::string translationKey = "action_type_" + type + "_display";
            actionTypeDisplayItems.push_back(m_translator.get(translationKey).c_str());
        }
        ImGui::Combo("##ActionTypeCombo", &m_newButtonActionTypeIndex, actionTypeDisplayItems.data(), actionTypeDisplayItems.size());
        ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            ImGui::SetTooltip("%s", m_translator.get("action_type_tooltip").c_str());
        }

        // --- Row 4: Action Param ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(m_translator.get("action_param_label").c_str());

        ImGui::TableSetColumnIndex(1);
        // Logic for Action Param rendering within the second column
        std::string currentActionTypeStr = (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size())
                                               ? m_supportedActionTypes[m_newButtonActionTypeIndex]
                                               : "";
        bool isHotkeyAction = (currentActionTypeStr == "hotkey");
        bool isLaunchAppAction = (currentActionTypeStr == "launch_app");
        // Check if action type starts with "media_"
        bool isMediaAction = (currentActionTypeStr.rfind("media_", 0) == 0);

        // --- Action Param Input based on Type ---
        if (isMediaAction) {
            // --- Media Action: Display Disabled Input ---
            ImGui::BeginDisabled(true); // Start disabled state
            char disabledText[] = "N/A"; // Placeholder text
            // Use Push/PopItemWidth to control width even when disabled
            float mediaWidth = ImGui::GetContentRegionAvail().x - helpIconWidth;
            ImGui::PushItemWidth(mediaWidth > 0 ? mediaWidth : -FLT_MIN);
            ImGui::InputText("##ActionParamDisabled", disabledText, sizeof(disabledText), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            ImGui::EndDisabled(); // End disabled state
            ImGui::SameLine(); ImGui::TextDisabled("(?)"); // Keep help icon
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }
            
            // Clear the actual buffer if it contains data from a previous type
            if (strlen(m_newButtonActionParam) > 0) {
                m_newButtonActionParam[0] = '\0';
            }
            // Ensure hotkey capture mode is off if we switched to media type
            if (m_isCapturingHotkey) m_isCapturingHotkey = false;

        } else if (isHotkeyAction) {
            // --- Hotkey Action --- 
            // (Hotkey logic including checkbox, NewLine, PushItemWidth, InputText variants, PopItemWidth, SameLine, TextDisabled, Tooltip)
            ImGui::Checkbox("##ManualHotkeyCheckbox", &m_manualHotkeyEntry);
            ImGui::SameLine(); ImGui::TextUnformatted(m_translator.get("hotkey_manual_input_checkbox").c_str());
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_manual_input_tooltip").c_str()); }
            ImGui::NewLine();

            float hotkeyWidth = ImGui::GetContentRegionAvail().x - helpIconWidth;
            ImGui::PushItemWidth(hotkeyWidth > 0 ? hotkeyWidth : -FLT_MIN);
            if (m_manualHotkeyEntry) {
                m_isCapturingHotkey = false;
                ImGui::InputText("##ActionParamInputManual", m_newButtonActionParam, sizeof(m_newButtonActionParam));
            } else {
                if (m_isCapturingHotkey) {
                    char capturePlaceholder[256];
                    const char* promptFormat = m_translator.get("hotkey_capture_prompt").c_str();
                    snprintf(capturePlaceholder, sizeof(capturePlaceholder), promptFormat, m_newButtonActionParam);
                    ImGui::InputText("##ActionParamInputCapturing", capturePlaceholder, sizeof(capturePlaceholder), ImGuiInputTextFlags_ReadOnly);
                    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_capturing").c_str()); }
                    if (InputUtils::TryCaptureHotkey(m_newButtonActionParam, sizeof(m_newButtonActionParam))) {
                        m_isCapturingHotkey = false;
                    }
                    if (!ImGui::IsItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                        ImGui::SetKeyboardFocusHere(-1);
                    }
                } else {
                    ImGui::InputText("##ActionParamInput", m_newButtonActionParam, sizeof(m_newButtonActionParam));
                    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("hotkey_capture_tooltip_start").c_str()); }
                    if (ImGui::IsItemClicked()) { m_isCapturingHotkey = true; }
                }
            }
            ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }

        } else { // Handles launch_app, open_url, etc.
            // --- Standard Input for other action types ---
             // Ensure hotkey capture mode is off
            if (m_isCapturingHotkey) m_isCapturingHotkey = false;
            float browseButtonWidth = ImGui::CalcTextSize(m_translator.get("browse_button_label").c_str()).x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
            float standardInputWidth = ImGui::GetContentRegionAvail().x - helpIconWidth - (isLaunchAppAction ? browseButtonWidth : 0.0f);
            ImGui::PushItemWidth(standardInputWidth > 0 ? standardInputWidth : -FLT_MIN);
            ImGui::InputText("##ActionParamInput", m_newButtonActionParam, sizeof(m_newButtonActionParam));
            ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("action_param_tooltip").c_str()); }

            if (isLaunchAppAction) {
                ImGui::SameLine();
                if (ImGui::Button(m_translator.get("browse_button_label").c_str())) {
                    const char* key = m_editingButtonId.empty() ? "SelectAppDlgKey_Add" : "SelectAppDlgKey_Edit";
                    const char* title = m_translator.get("select_app_dialog_title").c_str();
                    const char* filters = ".exe{,.*}";
                    IGFD::FileDialogConfig config; config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
                }
            }
        }
        // If action type changes away from hotkey, ensure capture mode is off (already handled in branches)
        // if (m_isCapturingHotkey && !isHotkeyAction && !isMediaAction) { m_isCapturingHotkey = false; }

        // --- Row 5: Icon Path ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(m_translator.get("button_icon_label").c_str());

        ImGui::TableSetColumnIndex(1);
        float iconBrowseButtonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemSpacing.x * 2.0f; // Width for "..."
        float iconInputWidth = ImGui::GetContentRegionAvail().x - helpIconWidth - iconBrowseButtonWidth;
        ImGui::PushItemWidth(iconInputWidth > 0 ? iconInputWidth : -FLT_MIN);
        ImGui::InputText("##IconPath", m_newButtonIconPath, IM_ARRAYSIZE(m_newButtonIconPath));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("...##IconBrowse")) { // Shortened label for browse
             const char* key = "SelectIconDlgKey_AddEdit";
             const char* title = "Select Button Icon";
             const char* filters = ".*"; // Common image filters
             IGFD::FileDialogConfig config; config.path = ".";
             config.userDatas = (void*)"AddEditIconPath";
             ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
        }
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", m_translator.get("button_icon_tooltip").c_str()); }

        // --- END TABLE LAYOUT ---
        ImGui::EndTable();
    }

    // --- Buttons after the table ---
    const char* submitLabel = isEditing ? m_translator.get("save_changes_button_label").c_str() : m_translator.get("add_button_label").c_str();
    if (ImGui::Button(submitLabel)) {
        ButtonConfig buttonData;
        // Populate common fields
        buttonData.name = m_newButtonName;
        if (m_newButtonActionTypeIndex >= 0 && m_newButtonActionTypeIndex < m_supportedActionTypes.size()) {
             buttonData.action_type = m_supportedActionTypes[m_newButtonActionTypeIndex];
        } else {
            buttonData.action_type = ""; // Handle error or default
            std::cerr << "Warning: Invalid action type index." << std::endl;
        }
        buttonData.action_param = m_newButtonActionParam;
        buttonData.icon_path = m_newButtonIconPath; // Assign icon path

        if (isEditing) {
            // --- Update Logic ---
            buttonData.id = m_editingButtonId; // Use the stored ID for update
            // Basic validation for updated button (ensure name is not empty)
            if (buttonData.name.empty()) {
                std::cerr << "Error: Updated button name cannot be empty." << std::endl;
            } else if (m_configManager.updateButton(m_editingButtonId, buttonData)) {
                std::cout << "Button updated successfully: " << m_editingButtonId << std::endl;
                if (m_configManager.saveConfig()) {
                    std::cout << "Configuration saved after updating button." << std::endl;
                    // Clear fields and exit edit mode
                    m_newButtonId[0] = '\0'; m_newButtonName[0] = '\0'; m_newButtonActionTypeIndex = -1;
                    m_newButtonActionParam[0] = '\0'; m_newButtonIconPath[0] = '\0';
                    m_editingButtonId = ""; // Exit edit mode
                } else { 
                    std::cerr << "Error: Failed to save configuration after updating button." << std::endl; 
                }
            } else {
                 std::cerr << "Error: Failed to update button " << m_editingButtonId << " (check console)." << std::endl;
            }
        } else {
            // --- Add Logic ---
            buttonData.id = m_newButtonId; // Use the ID from the input field
             // Basic validation: Check for empty ID or Name
            if (buttonData.id.empty() || buttonData.name.empty()) {
                std::cerr << "Error: Cannot add button with empty ID or Name." << std::endl;
            } else if (m_configManager.addButton(buttonData)) {
                std::cout << m_translator.get("button_added_success_log") << buttonData.id << std::endl;
                if (m_configManager.saveConfig()) {
                     std::cout << m_translator.get("config_saved_add_log") << std::endl;
                     m_newButtonId[0] = '\0';
                     m_newButtonName[0] = '\0';
                     m_newButtonActionTypeIndex = -1; // Reset index
                     m_newButtonActionParam[0] = '\0';
                     m_newButtonIconPath[0] = '\0'; // Clear icon path field
                } else {
                    std::cerr << m_translator.get("config_save_fail_add_log") << std::endl;
                }
            } else {
                std::cerr << m_translator.get("add_button_fail_log") << std::endl;
            }
        }
    }

    // --- ADDED: Cancel Button (only in edit mode) ---
    if (isEditing) {
        ImGui::SameLine();
        if (ImGui::Button(m_translator.get("cancel_button_label").c_str())) {
             // Clear fields and exit edit mode
             m_newButtonId[0] = '\0'; m_newButtonName[0] = '\0'; m_newButtonActionTypeIndex = -1;
             m_newButtonActionParam[0] = '\0'; m_newButtonIconPath[0] = '\0';
             m_editingButtonId = ""; // Exit edit mode
             std::cout << "Edit cancelled for button ID: " << m_editingButtonId << std::endl; // Note: m_editingButtonId is empty now
        }
    }


    // --- Delete Confirmation Modal --- (Remains the same)
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

    // --- MODIFIED: ImGuiFileDialog Display and Handling ---
    std::string selectedFilePath;
    bool updateParam = false; // Combined flag for action param
    bool updateIcon = false;  // Combined flag for icon path

    // Define display settings for the dialog
    ImVec2 minSize = ImVec2(600, 400);
    ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);

    // Display the dialog if opened (for selecting App File)
    if (ImGuiFileDialog::Instance()->Display("SelectAppDlgKey_AddEdit", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) { // Action if OK button is pressed
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            const char* userDataStr = static_cast<const char*>(ImGuiFileDialog::Instance()->GetUserDatas());
            if (userDataStr && strcmp(userDataStr, "AddEditActionParam") == 0) {
                updateParam = true;
            } 
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Display the dialog if opened (for selecting Icon File)
    if (ImGuiFileDialog::Instance()->Display("SelectIconDlgKey_AddEdit", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) { 
            selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
             const char* userDataStr = static_cast<const char*>(ImGuiFileDialog::Instance()->GetUserDatas());
              if (userDataStr && strcmp(userDataStr, "AddEditIconPath") == 0) {
                updateIcon = true;
            } 
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Update the buffer outside the Display scope based on flags
    if (updateParam) {
         // Update m_newButtonActionParam regardless of mode (Add or Edit)
         strncpy(m_newButtonActionParam, selectedFilePath.c_str(), IM_ARRAYSIZE(m_newButtonActionParam) - 1);
         m_newButtonActionParam[IM_ARRAYSIZE(m_newButtonActionParam) - 1] = 0; // Ensure null termination
    }
    if (updateIcon) {
         // Update m_newButtonIconPath regardless of mode (Add or Edit)
         strncpy(m_newButtonIconPath, selectedFilePath.c_str(), IM_ARRAYSIZE(m_newButtonIconPath) - 1);
         m_newButtonIconPath[IM_ARRAYSIZE(m_newButtonIconPath) - 1] = 0; // Ensure null termination
    }

    ImGui::End(); // End of Configuration Window
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