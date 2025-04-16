#include "GridPaginationComponent.hpp"
#include "../../Managers/TranslationManager.hpp" // Include needed for get()
#include <string> // For std::to_string
#include <algorithm> // For std::max
#include <vector>   // For std::vector used in map value
#include <map>      // For std::map used in signature

// Use the namespace alias if defined, otherwise use full namespace
GridPaginationComponent::GridPaginationComponent(TranslationManager& translator)
    : m_translator(translator) {}

// Draw method now receives state from the caller
void GridPaginationComponent::Draw(int& currentPageIndex, int pageCount, const std::map<int, std::vector<std::vector<std::string>>>& pagesMap) {
    // Only draw controls if there's more than one page
    if (pageCount > 1) {
        ImGui::Separator(); // Add a separator before the controls
        float buttonWidth = 40.0f;
        // Use ImGui::GetContentRegionAvail().x which is generally safer than Max-Min
        float windowWidth = ImGui::GetContentRegionAvail().x;

        // Calculate total width needed for buttons and text (adjust 100.0f if needed)
        float approxTextWidth = 100.0f; // Approximate width for page text (can be calculated better if needed)
        float controlsWidth = buttonWidth * 2.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f + approxTextWidth;
        float startX = (windowWidth - controlsWidth) * 0.5f; // Center the controls
        // Ensure it doesn't go left of the initial cursor pos + padding
        startX = std::max(startX, ImGui::GetStyle().WindowPadding.x);

        ImGui::SetCursorPosX(startX);

        // Previous Page Button
        // Use PushID/PopID to avoid conflicts if multiple paginators exist
        ImGui::PushID("PaginationPrev");
        if (ImGui::ArrowButton("##PagePrev", ImGuiDir_Left)) {
             int targetPageIndex = currentPageIndex - 1;
             // Check bounds and page existence
             if (targetPageIndex >= 0) {
                 // Find the nearest existing page index <= targetPageIndex
                 auto it = pagesMap.upper_bound(targetPageIndex); // First element GREATER than target
                 if (it != pagesMap.begin()) {
                    --it; // Move to the element <= target
                    currentPageIndex = it->first; // Update the reference directly
                 }
                 // If no page <= target exists (shouldn't happen if target >= 0 and pagesMap not empty), stay put.
             }
             // Optional: Wrap around to last page if targetPageIndex < 0
             // else { if (!pagesMap.empty()) currentPageIndex = pagesMap.rbegin()->first; }
        }
        ImGui::PopID();

        ImGui::SameLine();

        // Page Indicator Text
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y); // Align text vertically better
        // Find the actual total number of pages based on the map size if different from pageCount? Or trust pageCount? Trust pageCount for now.
        std::string pageText = std::to_string(currentPageIndex + 1) + " / " + std::to_string(pageCount);
        float textWidth = ImGui::CalcTextSize(pageText.c_str()).x;
         // Center text in its allocated space (approxTextWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (approxTextWidth - textWidth) * 0.5f);
        ImGui::TextUnformatted(pageText.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y); // Restore Y cursor

        ImGui::SameLine();
        // Position next button based on where the text ended, approximately
        // Adjust spacing as needed. The startX calculation needs refinement if exact centering is crucial.
        ImGui::SetCursorPosX(startX + buttonWidth + ImGui::GetStyle().ItemSpacing.x + approxTextWidth + ImGui::GetStyle().ItemSpacing.x);

        // Next Page Button
        ImGui::PushID("PaginationNext");
        if (ImGui::ArrowButton("##PageNext", ImGuiDir_Right)) {
            int targetPageIndex = currentPageIndex + 1;
            // Check bounds and page existence
             if (targetPageIndex < pageCount) {
                  // Find the nearest existing page index >= targetPageIndex
                  auto it = pagesMap.lower_bound(targetPageIndex); // First element >= target
                  if (it != pagesMap.end()) {
                       currentPageIndex = it->first; // Update the reference directly
                  }
                  // If no page >= target exists (shouldn't happen if target < pageCount and map not empty), stay put.
             }
             // Optional: Wrap around to first page if targetPageIndex >= pageCount
             // else { if (!pagesMap.empty()) currentPageIndex = pagesMap.begin()->first; }
        }
         ImGui::PopID();
    }
}
