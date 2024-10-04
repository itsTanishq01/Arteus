#include "FileMenu.h"
#include "imgui.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <commdlg.h>
#include <algorithm>

std::vector<Tab> tabs; // Collection of tabs
int currentTabIndex = -1; // Index of the currently active tab

bool LoadTextFromFile(const wchar_t* filename, std::string& text) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}

bool SaveTextToFile(const wchar_t* filename, const std::string& text) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << text;
    return true;
}

bool OpenFileDialog(wchar_t* filename, DWORD maxFileNameLength) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = maxFileNameLength;
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameW(&ofn);
}

bool SaveFileDialog(wchar_t* filename, DWORD maxFileNameLength) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = maxFileNameLength;
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    return GetSaveFileNameW(&ofn);
}

void ShowFileMenu(bool& done) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                tabs.push_back(Tab{ "Untitled", "" });
                currentTabIndex = static_cast<int>(tabs.size()) - 1; // Switch to the new tab
            }
            if (ImGui::MenuItem("Open")) {
                wchar_t filename[256] = L"";
                if (OpenFileDialog(filename, sizeof(filename))) {
                    std::string file_text;
                    if (LoadTextFromFile(filename, file_text)) {
                        // Convert to std::string
                        std::wstring wFileName = filename;
                        std::string fullPath(wFileName.begin(), wFileName.end());
                        std::string fileName = fullPath.substr(fullPath.find_last_of("/\\") + 1);

                        // Add new tab and set it as current
                        tabs.push_back(Tab{ fileName, file_text });
                        currentTabIndex = static_cast<int>(tabs.size()) - 1; // Switch to the new tab
                    }
                    else {
                        ImGui::Text("Error loading file!");
                    }
                }
            }
            if (ImGui::MenuItem("Save")) {
                if (currentTabIndex >= 0 && currentTabIndex < static_cast<int>(tabs.size())) {
                    wchar_t filename[256] = L"";
                    if (SaveFileDialog(filename, sizeof(filename))) {
                        SaveTextToFile(filename, tabs[currentTabIndex].content);
                    }
                }
            }
            if (ImGui::MenuItem("Exit")) {
                done = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void RenderTabs() {
    if (ImGui::BeginTabBar("Tabs")) {
        for (int i = 0; i < tabs.size(); i++) {
            bool isOpen = true;
            if (ImGui::BeginTabItem(tabs[i].title.c_str(), &isOpen)) {
                currentTabIndex = i; // Update the current tab index when a tab is selected

                // Display the content of the current tab
                ImGui::InputTextMultiline("##text", &tabs[i].content[0], tabs[i].content.size() + 1, ImVec2(-FLT_MIN, -FLT_MIN),
                    ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
                    [](ImGuiInputTextCallbackData* data) {
                        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                            std::string* text = (std::string*)data->UserData;
                            text->resize(data->BufTextLen);
                            data->Buf = &((*text)[0]);
                        }
                        return 0;
                    }, (void*)&tabs[i].content);

                ImGui::EndTabItem();
            }

            // Handle tab closure
            if (!isOpen) {
                tabs.erase(tabs.begin() + i);

                if (i == currentTabIndex) {
                    // If there are no tabs left, set index to -1
                    if (tabs.empty()) {
                        currentTabIndex = -1;
                    }
                    else {
                        // Select the previous tab if available
                        currentTabIndex = (i > 0) ? i - 1 : 0; // Go to the previous tab or the first tab
                    }
                }
                else if (i < currentTabIndex) {
                    currentTabIndex--;
                }

                i--; // Adjust index after erasing
            }
        }
        ImGui::EndTabBar();
    }
}