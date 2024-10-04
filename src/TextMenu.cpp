#include "imgui.h"
#include "TextMenu.h"
#include <algorithm>
#include <vector>
#include <string>
#define _CRT_SECURE_NO_WARNINGS

// Show search and replace dialog
void ShowSearchReplaceDialog(bool* open, std::string& searchText, std::string& replaceText, std::string& text) {
    if (!*open) return;

    ImGui::OpenPopup("Search and Replace");

    if (ImGui::BeginPopupModal("Search and Replace", open)) {
        // Create buffers for search and replace text
        static char searchBuffer[256] = "";
        static char replaceBuffer[256] = "";

        // Copy std::string content to buffers
        strncpy_s(searchBuffer, sizeof(searchBuffer), searchText.c_str(), _TRUNCATE);
        strncpy_s(replaceBuffer, sizeof(replaceBuffer), replaceText.c_str(), _TRUNCATE);

        // Input for search text
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
            searchText = searchBuffer; // Update std::string with buffer content
        }

        // Input for replace text
        if (ImGui::InputText("Replace", replaceBuffer, sizeof(replaceBuffer))) {
            replaceText = replaceBuffer; // Update std::string with buffer content
        }

        ImGui::Spacing(); // Add some space between input fields and buttons

        // Buttons for replacing
        if (ImGui::Button("Replace##replace")) {
            if (!searchText.empty()) {
                size_t pos = text.find(searchText);
                if (pos != std::string::npos) {
                    text.replace(pos, searchText.length(), replaceText);
                    ImGui::Text("Replaced first occurrence."); // Feedback for single replacement
                }
                else {
                    ImGui::Text("Search term not found."); // Clear feedback when nothing is found
                }
            }
            else {
                ImGui::Text("Please enter a search term.");
            }
        }

        ImGui::SameLine(); // Align buttons side by side
        if (ImGui::Button("Replace All##replace_all")) {
            if (!searchText.empty()) {
                size_t pos = 0;
                bool found = false; // Flag to check if any replacements were made
                while ((pos = text.find(searchText, pos)) != std::string::npos) {
                    text.replace(pos, searchText.length(), replaceText);
                    pos += replaceText.length(); // Move past the replaced text
                    found = true; // Mark that a replacement was made
                }
                if (found) {
                    ImGui::Text("All occurrences replaced."); // Feedback for multiple replacements
                }
                else {
                    ImGui::Text("No occurrences found."); // Clear feedback when nothing is found
                }
            }
            else {
                ImGui::Text("Please enter a search term.");
            }
        }

        ImGui::EndPopup();
    }
}
