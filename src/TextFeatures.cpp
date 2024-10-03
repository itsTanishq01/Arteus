#include "TextFeatures.h"
#include "imgui.h"
#include <algorithm>
#include <Windows.h> // For clipboard operations
#include <vector>
#define _CRT_SECURE_NO_WARNINGS

// Show search and replace dialog
void ShowSearchReplaceDialog(bool* open, std::string& searchText, std::string& replaceText, std::string& text) {
    if (!*open) return;

    ImGui::OpenPopup("Search and Replace");

    if (ImGui::BeginPopupModal("Search and Replace", open)) {
        // Create buffers for search and replace text
        static std::vector<char> searchBuffer(256);
        static std::vector<char> replaceBuffer(256);

        // Copy std::string content to buffers
        strncpy_s(searchBuffer.data(), searchBuffer.size(), searchText.c_str(), _TRUNCATE);
        strncpy_s(replaceBuffer.data(), replaceBuffer.size(), replaceText.c_str(), _TRUNCATE);

        // Input for search text
        if (ImGui::InputText("Search", searchBuffer.data(), searchBuffer.size())) {
            searchText = searchBuffer.data(); // Update std::string with buffer content
        }

        // Input for replace text
        if (ImGui::InputText("Replace", replaceBuffer.data(), replaceBuffer.size())) {
            replaceText = replaceBuffer.data(); // Update std::string with buffer content
        }

        ImGui::Spacing(); // Add some space between input fields and buttons

        // Buttons for replacing
        if (ImGui::Button("Replace##replace")) {
            if (!searchText.empty()) {
                size_t pos = text.find(searchText);
                if (pos != std::string::npos) {
                    text.replace(pos, searchText.length(), replaceText);
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
                while ((pos = text.find(searchText, pos)) != std::string::npos) {
                    text.replace(pos, searchText.length(), replaceText);
                    pos += replaceText.length(); // Move past the replaced text
                }
            }
            else {
                ImGui::Text("Please enter a search term.");
            }
        }

        ImGui::EndPopup();
    }
}
