#include "FileMenu.h"
#include "imgui.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <commdlg.h>
#include <algorithm>
#include <shlobj.h> // For SHBrowseForFolder

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

static bool OpenFolderDialog(wchar_t* foldername, DWORD maxFolderNameLength) {
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = L"Select Folder";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != 0) {
        // Get the name of the folder
        SHGetPathFromIDListW(pidl, foldername);
        // Free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
        return true;
    }
    return false;
}

void ShowFileMenu(bool& done) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                std::string newTitle = "Untitled";
                int count = 1;

                while (std::any_of(tabs.begin(), tabs.end(), [&newTitle](const Tab& tab) {
                    return tab.title == newTitle;
                    })) {
                    newTitle = "Untitled (" + std::to_string(count) + ")";
                    count++;
                }

                tabs.push_back(Tab{ newTitle, "", L"" }); // Initialize with empty filename
                currentTabIndex = static_cast<int>(tabs.size()) - 1; // Switch to the new tab
            }
            if (ImGui::MenuItem("Open")) {
                wchar_t filename[256] = L"";
                if (OpenFileDialog(filename, sizeof(filename) / sizeof(wchar_t))) {
                    std::string file_text;
                    if (LoadTextFromFile(filename, file_text)) {
                        std::wstring wFileName = filename;
                        std::string fullPath(wFileName.begin(), wFileName.end());
                        std::string fileName = fullPath.substr(fullPath.find_last_of("/\\") + 1);

                        auto it = std::find_if(tabs.begin(), tabs.end(), [&fileName](const Tab& tab) {
                            return tab.title == fileName;
                            });

                        if (it != tabs.end()) {
                            currentTabIndex = std::distance(tabs.begin(), it);
                        }
                        else {
                            // Ensure fullPath is converted to std::wstring
                            std::wstring wFullPath = std::wstring(fullPath.begin(), fullPath.end());
                            tabs.push_back(Tab{ fileName, file_text, wFullPath }); // Store full path
                            currentTabIndex = static_cast<int>(tabs.size()) - 1; // Switch to the new tab
                        }
                    }
                    else {
                        ImGui::Text("Error loading file!");
                    }
                }
            }
            if (ImGui::MenuItem("Open Folder")) {
                wchar_t foldername[256] = L"";
                if (OpenFolderDialog(foldername, sizeof(foldername) / sizeof(wchar_t))) {
                    // Handle folder loading logic here
                    // For example, you can list all files in the folder and open them as tabs
                    WIN32_FIND_DATAW findFileData;
                    HANDLE hFind = FindFirstFileW((std::wstring(foldername) + L"\\*").c_str(), &findFileData);

                    if (hFind != INVALID_HANDLE_VALUE) {
                        do {
                            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                std::wstring filePath = std::wstring(foldername) + L"\\" + findFileData.cFileName;
                                std::string file_text;
                                if (LoadTextFromFile(filePath.c_str(), file_text)) {
                                    std::wstring wFileName = findFileData.cFileName;
                                    std::string fileName(wFileName.begin(), wFileName.end());

                                    auto it = std::find_if(tabs.begin(), tabs.end(), [&fileName](const Tab& tab) {
                                        return tab.title == fileName;
                                        });

                                    if (it == tabs.end()) {
                                        tabs.push_back(Tab{ fileName, file_text, filePath }); // Store full path
                                    }
                                }
                            }
                        } while (FindNextFileW(hFind, &findFileData) != 0);
                        FindClose(hFind);
                    }
                }
            }
            if (ImGui::MenuItem("Save")) {
                if (currentTabIndex >= 0 && currentTabIndex < static_cast<int>(tabs.size())) {
                    if (!tabs[currentTabIndex].filename.empty()) {
                        SaveTextToFile(tabs[currentTabIndex].filename.c_str(), tabs[currentTabIndex].content);
                    }
                    else {
                        wchar_t filename[256] = L"";
                        if (SaveFileDialog(filename, sizeof(filename) / sizeof(wchar_t))) {
                            tabs[currentTabIndex].filename = filename; // Save filename for future use
                            SaveTextToFile(filename, tabs[currentTabIndex].content);
                        }
                    }
                }
            }
            if (ImGui::MenuItem("Save As")) {
                if (currentTabIndex >= 0 && currentTabIndex < static_cast<int>(tabs.size())) {
                    wchar_t filename[256] = L"";
                    if (SaveFileDialog(filename, sizeof(filename) / sizeof(wchar_t))) {
                        tabs[currentTabIndex].filename = filename; // Save filename for future use
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

                if (tabs.empty()) {
                    currentTabIndex = -1;
                }
                else {
                    if (i == currentTabIndex) {
                        currentTabIndex = (i > 0) ? i - 1 : 0; // Go to previous tab or first tab
                    }
                    else if (i < currentTabIndex) {
                        currentTabIndex--;
                    }
                }

                i--; // Adjust index after erasing
            }
        }
        ImGui::EndTabBar();
    }
}
