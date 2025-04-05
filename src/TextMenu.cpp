#include "imgui.h"
#include "TextMenu.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cctype>

// Case-insensitive search function
size_t ci_find(const std::string& str, const std::string& substr, size_t pos = 0) {
    auto it = std::search(
        str.begin() + pos, str.end(),
        substr.begin(), substr.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return it == str.end() ? std::string::npos : std::distance(str.begin(), it);
}

// Function to find all occurrences of a substring
void findAllOccurrences(const std::string& text, const std::string& searchText, std::vector<SearchResult>& results, bool caseSensitive) {
    results.clear();
    size_t pos = 0;
    while (true) {
        size_t found = caseSensitive ? text.find(searchText, pos) : ci_find(text, searchText, pos);
        if (found == std::string::npos) break;
        results.push_back({ found, found + searchText.length() });
        pos = found + 1;
    }
}

// Show search and replace dialog
void ShowSearchReplaceDialog(bool* open, std::string& searchText, std::string& replaceText, std::string& text,
    std::vector<SearchResult>& searchResults, bool& caseSensitive, size_t& currentMatchIndex) {  // Changed int to size_t
    if (!*open) return;

    ImGui::OpenPopup("Search and Replace");

    if (ImGui::BeginPopupModal("Search and Replace", open)) {
        static char searchBuffer[256] = "";
        static char replaceBuffer[256] = "";

        // Use strncpy_s instead of strncpy
        strncpy_s(searchBuffer, searchText.c_str(), sizeof(searchBuffer) - 1);
        strncpy_s(replaceBuffer, replaceText.c_str(), sizeof(replaceBuffer) - 1);

        bool searchModified = ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
        if (searchModified) {
            searchText = searchBuffer;
            findAllOccurrences(text, searchText, searchResults, caseSensitive);
            currentMatchIndex = 0;
        }

        if (ImGui::InputText("Replace", replaceBuffer, sizeof(replaceBuffer))) {
            replaceText = replaceBuffer;
        }

        if (ImGui::Checkbox("Case Sensitive", &caseSensitive)) {
            findAllOccurrences(text, searchText, searchResults, caseSensitive);
            currentMatchIndex = 0;
        }

        ImGui::Text("Found %d matches", static_cast<int>(searchResults.size()));

        ImGui::Spacing();

        if (ImGui::Button("Replace##replace")) {
            if (!searchResults.empty()) {
                size_t pos = searchResults[currentMatchIndex].start;
                text.replace(pos, searchText.length(), replaceText);
                findAllOccurrences(text, searchText, searchResults, caseSensitive);
                if (currentMatchIndex >= searchResults.size()) {
                    currentMatchIndex = searchResults.size() - 1;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Replace All")) {
            size_t replacements = 0;
            for (auto it = searchResults.rbegin(); it != searchResults.rend(); ++it) {
                text.replace(it->start, searchText.length(), replaceText);
                replacements++;
            }
            findAllOccurrences(text, searchText, searchResults, caseSensitive);
            currentMatchIndex = 0;
            ImGui::Text("%zu occurrences replaced.", replacements);
        }

        ImGui::EndPopup();
    }
}