#include "FileHierarchy.h"
#include "imgui.h"
#include "FileMenu.h"
#include <algorithm>
#include <Windows.h>
#include <Shlobj.h>
#include <string>
#include <vector>
#include <filesystem>

// Current working directory
std::wstring currentWorkingDirectory = L"";

// Root hierarchy nodes
std::vector<FileNode> hierarchyNodes;

// Build file hierarchy from a root directory
void BuildFileHierarchy(const std::wstring& rootPath, std::vector<FileNode>& hierarchy) {
    hierarchy.clear();

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((rootPath + L"\\*").c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring filename = findFileData.cFileName;
            if (filename == L"." || filename == L"..") continue;

            bool isDirectory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            std::wstring fullPath = rootPath + L"\\" + filename;

            FileNode node;
            node.name = std::string(filename.begin(), filename.end());
            node.path = fullPath;
            node.isDirectory = isDirectory;
            node.isOpen = false;

            if (isDirectory) {
                BuildFileHierarchy(fullPath, node.children);
            }

            hierarchy.push_back(node);
        } while (FindNextFileW(hFind, &findFileData) != 0);

        FindClose(hFind);

        // Sort: directories first, then files alphabetically
        std::sort(hierarchy.begin(), hierarchy.end(), [](const FileNode& a, const FileNode& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
            });
    }
}

// Refresh the hierarchy
void RefreshHierarchy() {
    if (!currentWorkingDirectory.empty()) {
        BuildFileHierarchy(currentWorkingDirectory, hierarchyNodes);
    }
}

// Set the current working directory
void SetWorkingDirectory(const std::wstring& path) {
    currentWorkingDirectory = path;
    RefreshHierarchy();
}

// Recursively render file nodes
void RenderFileNode(FileNode& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (!node.isDirectory) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // Display the node without icons
    std::string displayName = node.name;
    bool isOpen = ImGui::TreeNodeEx(displayName.c_str(), flags);

    // Handle click to open file
    if (ImGui::IsItemClicked() && !node.isDirectory) {
        std::string fileContent;
        if (LoadTextFromFile(node.path.c_str(), fileContent)) {
            // Check if the file is already open in a tab
            auto it = std::find_if(tabs.begin(), tabs.end(), [&node](const Tab& tab) {
                return tab.filename == node.path;
                });

            if (it != tabs.end()) {
                currentTabIndex = std::distance(tabs.begin(), it);
            }
            else {
                tabs.push_back(Tab{ node.name, fileContent, node.path });
                currentTabIndex = static_cast<int>(tabs.size()) - 1;
            }
        }
    }

    // Context menu for files and folders
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            // Delete file or directory logic
            if (node.isDirectory) {
                // Implement directory deletion
            }
            else {
                // Implement file deletion
            }
            RefreshHierarchy();
        }

        if (node.isDirectory) {
            if (ImGui::MenuItem("New File")) {
                // Implement new file creation
                RefreshHierarchy();
            }
            if (ImGui::MenuItem("New Folder")) {
                // Implement new folder creation
                RefreshHierarchy();
            }
        }

        ImGui::EndPopup();
    }

    // Render children if directory is open
    if (node.isDirectory && isOpen) {
        for (auto& child : node.children) {
            RenderFileNode(child);
        }
        ImGui::TreePop();
    }
}

// Show the hierarchy window
void ShowHierarchyWindow() {
    ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse);

    // Button to select a folder
    if (ImGui::Button("Open Folder")) {
        wchar_t folderPath[MAX_PATH] = L"";
        BROWSEINFOW bi = { 0 };
        bi.lpszTitle = L"Select Folder";
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);

        if (pidl != 0) {
            SHGetPathFromIDListW(pidl, folderPath);
            SetWorkingDirectory(folderPath);

            // Free memory
            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                imalloc->Free(pidl);
                imalloc->Release();
            }
        }
    }

    ImGui::Separator();

    // Display current path
    if (!currentWorkingDirectory.empty()) {
        std::string path(currentWorkingDirectory.begin(), currentWorkingDirectory.end());
        ImGui::Text("Current: %s", path.c_str());
        ImGui::Separator();

        // Render the hierarchy
        for (auto& node : hierarchyNodes) {
            RenderFileNode(node);
        }
    }
    else {
        ImGui::Text("No folder selected");
    }

    ImGui::End();
}
