#pragma once

#include <imgui.h>
#include <string>
#include <map> // For pagesMap validation (optional but good)
#include <vector> // Required for vector in map value type
#include <string> // Required for string in vector
// #include "../../Managers/TranslationManager.hpp" // Include only if definition needed here, likely not.

// Forward declaration for the non-namespaced TranslationManager
class TranslationManager;

class GridPaginationComponent {
public:
    // Constructor takes dependencies
    GridPaginationComponent(TranslationManager& translator);

    // Draw method takes the current state by reference/pointer to modify it
    // Takes pageCount and the actual map of pages for validation
    // Note: Using std::vector<std::vector<std::string>> for the map value type
    void Draw(int& currentPageIndex, int pageCount, const std::map<int, std::vector<std::vector<std::string>>>& pagesMap);

private:
    TranslationManager& m_translator;
    // No internal state needed for simple pagination
};
