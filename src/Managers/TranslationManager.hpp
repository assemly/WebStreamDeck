#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <map>
#include <vector> // For available languages

using json = nlohmann::json;

class TranslationManager {
public:
    // Constructor, loads default language
    explicit TranslationManager(const std::string& langFolderPath = "assets/lang", const std::string& defaultLang = "zh");

    // Get translated string for a given key
    const std::string& get(const std::string& key);

    // Switch language
    bool setLanguage(const std::string& langCode);

    // Get current language code
    const std::string& getCurrentLanguage() const;

    // Get available language codes (detected from files)
    const std::vector<std::string>& getAvailableLanguages() const;


private:
    std::string m_langFolderPath;
    std::string m_currentLanguage;
    json m_translations; // Current loaded translations
    std::string m_fallbackString = "???"; // String to return if key not found
    std::map<std::string, std::string> m_cachedStrings; // Cache for retrieved strings to return const ref
    std::vector<std::string> m_availableLanguages;

    // Load translations from a file for a specific language code
    bool loadLanguage(const std::string& langCode);

    // Scan the lang folder for available language files (.json)
    void detectAvailableLanguages();
};