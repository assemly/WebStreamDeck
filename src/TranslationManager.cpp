#include "TranslationManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem> // Requires C++17

namespace fs = std::filesystem;

TranslationManager::TranslationManager(const std::string& langFolderPath, const std::string& defaultLang)
    : m_langFolderPath(langFolderPath)
{
    detectAvailableLanguages();
    if (!setLanguage(defaultLang)) {
        std::cerr << "Warning: Failed to load default language '" << defaultLang << "'.";
        // Try loading English as a fallback if available
        if (std::find(m_availableLanguages.begin(), m_availableLanguages.end(), "en") != m_availableLanguages.end()) {
             if (setLanguage("en")) {
                 std::cerr << " Loaded 'en' instead." << std::endl;
             } else {
                 std::cerr << " Failed to load 'en' fallback either." << std::endl;
                 m_currentLanguage = ""; // Indicate no language loaded
             }
        } else {
             std::cerr << " No fallback language found." << std::endl;
             m_currentLanguage = "";
        }
    }
}

void TranslationManager::detectAvailableLanguages() {
    m_availableLanguages.clear();
    try {
        if (fs::exists(m_langFolderPath) && fs::is_directory(m_langFolderPath)) {
            for (const auto& entry : fs::directory_iterator(m_langFolderPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    m_availableLanguages.push_back(entry.path().stem().string());
                     std::cout << "Detected language file: " << entry.path().filename() << std::endl;
                }
            }
        } else {
             std::cerr << "Language folder not found or not a directory: " << m_langFolderPath << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error accessing language folder: " << e.what() << std::endl;
    }
     std::sort(m_availableLanguages.begin(), m_availableLanguages.end()); // Keep it sorted
}


bool TranslationManager::loadLanguage(const std::string& langCode) {
    fs::path langFilePath = fs::path(m_langFolderPath) / (langCode + ".json");
    if (!fs::exists(langFilePath)) {
        std::cerr << "Error: Language file not found: " << langFilePath << std::endl;
        return false;
    }

    std::ifstream langFile(langFilePath);
    if (!langFile.is_open()) {
        std::cerr << "Error: Could not open language file: " << langFilePath << std::endl;
        return false;
    }

    try {
        langFile >> m_translations;
        langFile.close();
        m_cachedStrings.clear(); // Clear cache when new language is loaded
        std::cout << "Successfully loaded language: " << langCode << std::endl;
        return true;
    } catch (json::parse_error& e) {
        std::cerr << "Error parsing language file: " << langFilePath << "\nMessage: " << e.what() << std::endl;
        m_translations = json::object(); // Clear translations on error
        return false;
    } catch (const std::exception& e) {
         std::cerr << "Error loading language file " << langFilePath << ": " << e.what() << std::endl;
         m_translations = json::object();
         return false;
    }
}

bool TranslationManager::setLanguage(const std::string& langCode) {
    if (loadLanguage(langCode)) {
        m_currentLanguage = langCode;
        return true;
    }
    return false;
}

const std::string& TranslationManager::get(const std::string& key) {
    // Check cache first
    auto it_cache = m_cachedStrings.find(key);
    if (it_cache != m_cachedStrings.end()) {
        return it_cache->second;
    }

    // If not in cache, look in the loaded JSON translations
    try {
        auto it_json = m_translations.find(key);
        if (it_json != m_translations.end() && it_json->is_string()) {
            // Key found in JSON, get the value
            std::string value = it_json->get<std::string>();
            // Insert into the mutable cache and return reference to inserted element
            // Use emplace for potential efficiency
            auto result = m_cachedStrings.emplace(key, std::move(value)); 
            return result.first->second; // Return reference to the inserted string in the map
        } else {
            // Key not found in JSON or not a string
            if (!m_currentLanguage.empty()) { // Only warn if a language was actually loaded
                 std::cerr << "Warning: Translation key not found or not a string: '" << key << "' in language '" << m_currentLanguage << "'" << std::endl;
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "Error accessing translation key '" << key << "': " << e.what() << std::endl;
    }

     // Fallback: Key not found or error occurred
     // Insert the fallback string into the cache and return reference to it
     auto result = m_cachedStrings.emplace(key, m_fallbackString); // Use fallback string directly
     return result.first->second;
}

const std::string& TranslationManager::getCurrentLanguage() const {
    return m_currentLanguage;
}

const std::vector<std::string>& TranslationManager::getAvailableLanguages() const {
    return m_availableLanguages;
}
