// Define this in exactly one .c or .cpp file
#define MA_IMPLEMENTATION
#include <miniaudio.h>

#include "SoundPlaybackService.hpp"
#include <filesystem> // For checking file existence
#include <iostream>   // For std::cerr

SoundPlaybackService::SoundPlaybackService() : m_isInitialized(false) {
    // Constructor - engine is initialized in init()
}

SoundPlaybackService::~SoundPlaybackService() {
    // Ensure shutdown is called if not already
    if (m_isInitialized) {
        shutdown();
    }
}

// Helper to check miniaudio results and print errors
bool SoundPlaybackService::check(ma_result result, const std::string& message) {
    if (result != MA_SUCCESS) {
        std::cerr << "[SoundService] MiniAudio Error: " << message << " (Code: " << result << ", " << ma_result_description(result) << ")" << std::endl;
        return false;
    }
    return true;
}

bool SoundPlaybackService::init() {
    if (m_isInitialized) {
        std::cout << "[SoundService] Already initialized." << std::endl;
        return true;
    }

    ma_result result = ma_engine_init(NULL, &m_engine);
    if (!check(result, "Failed to initialize audio engine")) {
        return false;
    }

    m_isInitialized = true;
    std::cout << "[SoundService] Audio engine initialized successfully." << std::endl;
    return true;
}

// Renamed from loadSound, now just registers the path
bool SoundPlaybackService::registerSound(const std::string& name, const std::string& filepath) {
    if (!m_isInitialized) {
        std::cerr << "[SoundService] Error: Cannot register sound, engine not initialized." << std::endl;
        return false;
    }

    // Check if file exists before trying to load
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "[SoundService] Error: Sound file not found: " << filepath << std::endl;
        return false;
    }

    // Check if sound name already exists (optional, could overwrite)
    if (m_soundFilepaths.count(name)) {
        std::cerr << "[SoundService] Warning: Sound name '" << name << "' already registered. Overwriting path." << std::endl;
    }
    
    m_soundFilepaths[name] = filepath; // Store or overwrite the path
    std::cout << "[SoundService] Sound registered: '" << name << "' -> " << filepath << std::endl;
    return true;
}

// Modified to use ma_engine_play_sound for fire-and-forget playback
void SoundPlaybackService::playSound(const std::string& name) {
    if (!m_isInitialized) {
        std::cerr << "[SoundService] Error: Cannot play sound, engine not initialized." << std::endl;
        return;
    }

    auto it = m_soundFilepaths.find(name);
    if (it != m_soundFilepaths.end()) {
        const char* filepath = it->second.c_str();
        // Fire and forget playback - miniaudio handles the resource management
        ma_result result = ma_engine_play_sound(&m_engine, filepath, NULL); 
        if (!check(result, "Failed to play sound: " + name + " (" + filepath + ")")) {
            // Handle error if needed, e.g., log it
        }
        // Logging successful start might be too noisy for rapid playback
        // std::cout << "[SoundService] Started playing sound: " << name << std::endl;
    } else {
        std::cerr << "[SoundService] Error: Sound name not registered: " << name << std::endl;
    }
}

// Modified to remove ma_sound unloading
void SoundPlaybackService::shutdown() {
    if (!m_isInitialized) {
        return;
    }

    std::cout << "[SoundService] Shutting down..." << std::endl;

    // Clear the map of file paths
    m_soundFilepaths.clear();

    // Uninitialize the engine
    ma_engine_uninit(&m_engine);
    m_isInitialized = false;
    std::cout << "[SoundService] Audio engine shut down." << std::endl;
}
