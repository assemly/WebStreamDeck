#pragma once

#include <string>
#include <vector>
#include <map>
#include <miniaudio.h> // Include miniaudio header
#include <iostream> // For logging

class SoundPlaybackService {
public:
    SoundPlaybackService();
    ~SoundPlaybackService();

    // Initialize the audio engine
    bool init();

    // Register a sound file path with a name
    // Returns true on success, false on failure (e.g., file not found).
    bool registerSound(const std::string& name, const std::string& filepath);

    // Play a previously registered sound by its name (fire and forget)
    void playSound(const std::string& name);

    // Uninitialize the audio engine and release resources
    void shutdown();

private:
    ma_engine m_engine;
    bool m_isInitialized = false;

    // Map to store file paths for registered sounds
    std::map<std::string, std::string> m_soundFilepaths;

    // Helper to check miniaudio results
    bool check(ma_result result, const std::string& message);
}; 