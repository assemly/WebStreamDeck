#include "ActionExecutionService.hpp"
#include "../Utils/InputUtils.hpp" // Needed for SimulateMediaKeyPress and audio
#include "../Utils/StringUtils.hpp" // Added
#include <iostream> // For error reporting
#include <string> // Needed for wstring conversion, etc.
#include <vector> // Needed for wstring conversion buffer & INPUT array, and melody sequence
#include <algorithm> // For std::transform
#include <sstream>   // For splitting string
#include <map>       // For key mapping
#include <thread>    // For starting async playback
#include <chrono>    // For sleep duration

// Platform specific includes for actions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // For ShellExecute
#else
// Add includes for Linux/macOS process creation and URL opening later if needed
#include <cstdlib> // For system()
#endif

// --- Helper Functions (Moved to StringUtils) --- 

// --- ActionExecutionService Implementation --- 

// Constructor updated to take SoundPlaybackService reference
ActionExecutionService::ActionExecutionService(SoundPlaybackService& soundService)
    : m_soundService(soundService) {}

// Executes an action based on explicit type and parameter
void ActionExecutionService::executeAction(const std::string& actionType, const std::string& actionParam) {
    std::cout << "[Exec Service] Executing action: Type='" << actionType 
              << "', Param='" << actionParam << "'" << std::endl;

    // --- Action Logic (using actionType and actionParam directly) ---
    if (actionType == "launch_app") {
        executeLaunchApp(actionParam);
    } else if (actionType == "open_url") {
        executeOpenUrl(actionParam);
    } else if (actionType == "hotkey") {
        executeHotkey(actionParam);
    } 
    // --- Media Key Actions --- 
    else if (actionType == "media_volume_up") {
        std::cout << "Executing: Media Volume Up (Core Audio) x2" << std::endl;
        bool success1 = InputUtils::IncreaseMasterVolume();
        bool success2 = InputUtils::IncreaseMasterVolume(); 
        if (!success1 || !success2) {
             std::cerr << "Failed to increase volume (at least one step failed)." << std::endl;
        }
    }
    else if (actionType == "media_volume_down") {
        std::cout << "Executing: Media Volume Down (Core Audio)" << std::endl;
        if (!InputUtils::DecreaseMasterVolume()) {
             std::cerr << "Failed to decrease volume." << std::endl;
        }
    }
    else if (actionType == "media_mute") {
        std::cout << "Executing: Media Mute Toggle (Core Audio)" << std::endl;
        if (!InputUtils::ToggleMasterMute()) {
             std::cerr << "Failed to toggle mute." << std::endl;
        }
    }
    else if (actionType == "media_play_pause") {
        std::cout << "Executing: Media Play/Pause (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_PLAY_PAUSE);
    }
    else if (actionType == "media_next_track") {
        std::cout << "Executing: Media Next Track (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_NEXT_TRACK);
    }
    else if (actionType == "media_prev_track") {
        std::cout << "Executing: Media Previous Track (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_PREV_TRACK);
    }
    else if (actionType == "media_stop") {
        std::cout << "Executing: Media Stop (Simulate Key)" << std::endl;
        InputUtils::SimulateMediaKeyPress(VK_MEDIA_STOP);
    }
    // --- REMOVED: Original Chinese Musical Notes Actions ---
    // --- ADDED: New individual sound actions based on WAV files ---
    else if (actionType == "play_gong_c3") {
        m_soundService.playSound("gong_c3");
    } else if (actionType == "play_gong_c4") {
        m_soundService.playSound("gong_c4");
    } else if (actionType == "play_gong_c5") {
        m_soundService.playSound("gong_c5");
    } else if (actionType == "play_shang_d3") {
        m_soundService.playSound("shang_d3");
    } else if (actionType == "play_shang_d4") {
        m_soundService.playSound("shang_d4");
    } else if (actionType == "play_shang_d5") {
        m_soundService.playSound("shang_d5");
    } else if (actionType == "play_jiao_e3") {
        m_soundService.playSound("jiao_e3");
    } else if (actionType == "play_jiao_e4") {
        m_soundService.playSound("jiao_e4");
    } else if (actionType == "play_jiao_e5") {
        m_soundService.playSound("jiao_e5");
    } else if (actionType == "play_qingjiao_f3") {
        m_soundService.playSound("qingjiao_f3");
    } else if (actionType == "play_qingjiao_f4") {
        m_soundService.playSound("qingjiao_f4");
    } else if (actionType == "play_qingjiao_f5") {
        m_soundService.playSound("qingjiao_f5");
    } else if (actionType == "play_zheng_g3") { // Assuming zhi is zheng
        m_soundService.playSound("zheng_g3");
    } else if (actionType == "play_zheng_g4") {
        m_soundService.playSound("zheng_g4");
    } else if (actionType == "play_zheng_g5") {
        m_soundService.playSound("zheng_g5");
    } else if (actionType == "play_yu_a3") {
        m_soundService.playSound("yu_a3");
    } else if (actionType == "play_yu_a4") {
        m_soundService.playSound("yu_a4");
    } else if (actionType == "play_yu_a5") {
        m_soundService.playSound("yu_a5");
    } else if (actionType == "play_biangong_b3") {
        m_soundService.playSound("biangong_b3");
    } else if (actionType == "play_biangong_b4") {
        m_soundService.playSound("biangong_b4");
    } else if (actionType == "play_biangong_b5") {
        m_soundService.playSound("biangong_b5");
    }
    // --- RE-ADDED: Melody Playback Action ---
    else if (actionType == "play_melody_qinghuaci") {
        std::cout << "[Exec Service] Starting melody playback in background thread..." << std::endl;
        // Launch the melody playback in a separate thread and detach it
        // so it doesn't block the main thread.
        std::thread melodyThread(&ActionExecutionService::playMelodyQinghuaciAsync, this);
        melodyThread.detach();
    }
    // --- END of Added Actions ---
    else {
        std::cerr << "Error: Unknown action type '" << actionType << "' requested." << std::endl;
    }
}

// --- Private Helper Method Implementations (Moved from ActionExecutor.cpp) --- 

bool ActionExecutionService::executeLaunchApp(const std::string& path) {
#ifdef _WIN32
    // Convert UTF-8 path std::string to std::wstring for ShellExecuteW
    std::wstring wPath = StringUtils::Utf8ToWide(path); // Use StringUtils namespace
    if (wPath.empty() && !path.empty()) { // Check if conversion failed
        std::cerr << "Error executing launch_app: Failed to convert path to wstring: " << path << std::endl;
        return false;
    }

    HINSTANCE result = ShellExecuteW(NULL, L"open", wPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result <= 32) { 
        // Log the original path for clarity, as wPath might not display well in std::cerr
        std::cerr << "Error executing launch_app: Failed to launch '" << path << "' (ShellExecuteW Error code: " << (intptr_t)result << ")" << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "Error: launch_app action is currently only supported on Windows." << std::endl;
    // Example for Linux/macOS (less robust):
    // std::string command = "xdg-open " + path; // Linux
    // std::string command = "open " + path; // macOS
    // int result = system(command.c_str());
    // return result == 0;
    return false;
#endif
}

bool ActionExecutionService::executeOpenUrl(const std::string& url) {
#ifdef _WIN32
    std::wstring wUrl = StringUtils::Utf8ToWide(url); // Use StringUtils namespace
    if (wUrl.empty() && !url.empty()) { // Check if conversion failed
         std::cerr << "Error executing open_url: Failed to convert URL to wstring: " << url << std::endl;
        return false;
    }
    HINSTANCE result = ShellExecuteW(NULL, L"open", wUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result <= 32) { 
        std::cerr << "Error executing open_url: Failed to open '" << url << "' (ShellExecuteW Error code: " << (intptr_t)result << ")" << std::endl;
        return false;
    }
    return true;
#else
    // Example for Linux/macOS
    std::string command;
    #ifdef __linux__
        command = "xdg-open \"" + url + "\""; // Use xdg-open on Linux, quote URL
    #elif __APPLE__
        command = "open \"" + url + "\""; // Use open on macOS, quote URL
    #else
         std::cerr << "Error: open_url action is not implemented for this platform." << std::endl;
        return false;
    #endif
    int res = system(command.c_str());
    if (res != 0) {
        std::cerr << "Error executing open_url: Failed to execute command '" << command << "' (return code: " << res << ")" << std::endl;
        return false;
    }
    return true;
#endif
}

bool ActionExecutionService::executeHotkey(const std::string& hotkeyString) {
#ifdef _WIN32
    std::vector<INPUT> inputs;
    std::vector<WORD> keyCodes;
    std::stringstream ss(hotkeyString);
    std::string segment;

    // Parse the hotkey string (e.g., "CTRL+ALT+T")
    while (std::getline(ss, segment, '+')) {
        WORD vk = StringUtils::StringToVkCode(segment); // Use StringUtils namespace
        if (vk != 0) {
            keyCodes.push_back(vk);
        } else {
            std::cerr << "Error executing hotkey: Invalid key segment '" << segment << "' in hotkey string: " << hotkeyString << std::endl;
            return false;
        }
    }

    if (keyCodes.empty()) {
         std::cerr << "Error executing hotkey: No valid key codes parsed from string: " << hotkeyString << std::endl;
        return false;
    }

    // Press keys down
    inputs.resize(keyCodes.size());
    for (size_t i = 0; i < keyCodes.size(); ++i) {
        inputs[i].type = INPUT_KEYBOARD;
        inputs[i].ki.wVk = keyCodes[i];
        inputs[i].ki.dwFlags = 0; // Key down
        inputs[i].ki.time = 0;
        inputs[i].ki.dwExtraInfo = 0;
    }
    UINT sentDown = SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
    if (sentDown != inputs.size()) {
        std::cerr << "Error executing hotkey: SendInput failed to press keys down (Error code: " << GetLastError() << ")" << std::endl;
        // Attempt to release any keys that might have been pressed
         for (size_t i = 0; i < keyCodes.size(); ++i) inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
         SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
        return false;
    }

     // Give a small delay before releasing keys, sometimes needed
    Sleep(20); // 20ms delay

    // Release keys up (in reverse order)
    for (size_t i = 0; i < keyCodes.size(); ++i) {
        inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; // Key up
    }
    // It's generally recommended to release in reverse order of press
    std::reverse(inputs.begin(), inputs.end()); 
    UINT sentUp = SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
     if (sentUp != inputs.size()) {
        std::cerr << "Error executing hotkey: SendInput failed to release keys up (Error code: " << GetLastError() << ")" << std::endl;
        // This is less critical, but logging is useful
        return false; // Consider if failure to release should be a hard fail
    }

    return true;
#else
    std::cerr << "Error: hotkey action is currently only supported on Windows." << std::endl;
    return false;
#endif
}

// <<< RE-ADDED: Implementation for asynchronous melody playback >>>
void ActionExecutionService::playMelodyQinghuaciAsync() {
    std::cout << "[Melody Thread] Playback started." << std::endl;
    
    // 青花瓷主歌片段（科学记谱法）
    const std::vector<std::pair<std::string, int>> melody = {
          // Phrase 1: 天青色等烟雨 (5 5 3 | 2 3 6_) - Revised with correct low A and timing attempt
        {"zheng_g4",386}, {"zheng_g4", 386}, {"jiao_e4", 386}, // 5 5 3
        {"shang_d4", 386}, {"jiao_e4",386}, {"yu_a3", 773},   // 2 3 6_ (Low A, duration guess)


        // // 副歌"2 3 5 3 | 2"（而我在等你）
        {"shang_d4", 386}, {"jiao_e4", 386},{"zheng_g4", 386} ,{"jiao_e4", 386} ,{"shang_d4", 1546},


        // //  5 5 3 | 2 3 5_ 炊烟袅袅升起
        {"zheng_g4",386}, {"zheng_g4", 386}, {"jiao_e4", 386}, // 5 5 3
        {"shang_d4", 386}, {"jiao_e4",386}, {"zheng_g3",773},   

        // // 2 3 5 2 | 1 隔江千万里
         {"shang_d4", 386}, {"jiao_e4", 386},{"zheng_g4", 386} ,{"jiao_e4", 386} , {"gong_c4",1546},

          // Phrase 5: 在瓶底书汉隶 仿前朝的飘逸 (0 1 2 3 | 5 6 5 4) - Ignoring 0 rest
        {"gong_c4", 386}, {"shang_d4", 386}, {"jiao_e4", 386},           // 1 2 3
        {"zheng_g4", 386}, {"yu_a4", 386}, {"zheng_g4", 386}, {"qingjiao_f4", 386}, // 5 6 5 4 (using f4 for 4)
        {"zheng_g4", 386}, {"jiao_e4", 386}, {"jiao_e4", 386}, {"shang_d4", 386}, {"shang_d4", 1546}, // 5 3 3 2 | 2
       // 就当我 0 1 2 1
        {"gong_c4", 386}, {"shang_d4", 386}, {"gong_c4", 386},
        // 为遇见// 1 2 1 2
        {"gong_c4", 386}, {"shang_d4", 386},{"gong_c4", 386}, {"shang_d4", 386},
        //  2 3 5 3 3
         {"shang_d4", 386}, {"jiao_e4", 773},{"zheng_g4",386}, {"jiao_e4",386},{"jiao_e4",1158}


    };

    // 动态速度控制（模拟原曲的rubato节奏）
    int baseTempo = 250; 
    for (const auto& notePause : melody) {
        const std::string& note = notePause.first;
        int dynamicDuration = notePause.second * (rand() % 20 + 90) / 100; // ±10%随机波动
        const std::chrono::milliseconds pauseDuration(dynamicDuration);
        
        m_soundService.playSound(note);
        std::this_thread::sleep_for(pauseDuration);
    }
    std::cout << "[Melody Thread] Playback finished." << std::endl;
} 