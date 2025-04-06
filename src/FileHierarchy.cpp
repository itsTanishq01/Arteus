#include "FileHierarchy.h"
#include "imgui.h"
#include "FileMenu.h"
#include <algorithm>
#include <Windows.h>
#include <Shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <cctype>

// Current working directory
std::wstring currentWorkingDirectory = L"";

// Root hierarchy nodes
std::vector<FileNode> hierarchyNodes;

// Bookmarked paths
std::unordered_set<std::wstring> bookmarkedPaths;

// File search state
bool isSearchActive = false;
std::vector<FileSearchResult> currentSearchResults;
std::string currentSearchTerm;

// File extension to FileType mapping
std::unordered_map<std::wstring, FileType> extensionToFileType = {
    {L".txt", FileType::Text},
    {L".md", FileType::Text},
    {L".c", FileType::Source},
    {L".cpp", FileType::Source},
    {L".cc", FileType::Source},
    {L".h", FileType::Header},
    {L".hpp", FileType::Header},
    {L".jpg", FileType::Image},
    {L".jpeg", FileType::Image},
    {L".png", FileType::Image},
    {L".bmp", FileType::Image},
    {L".gif", FileType::Image},
    {L".mp3", FileType::Audio},
    {L".wav", FileType::Audio},
    {L".ogg", FileType::Audio},
    {L".mp4", FileType::Video},
    {L".avi", FileType::Video},
    {L".mkv", FileType::Video},
    {L".doc", FileType::Document},
    {L".docx", FileType::Document},
    {L".pdf", FileType::Document},
    {L".zip", FileType::Archive},
    {L".rar", FileType::Archive},
    {L".7z", FileType::Archive},
    {L".exe", FileType::Executable},
    {L".dll", FileType::Executable}
};

// Maps FileType to icon string (using simple ASCII icons)
std::unordered_map<FileType, const char*> fileTypeToIcon = {
    {FileType::Unknown, "?"},
    {FileType::Text, "T"},      // Text icon
    {FileType::Source, "C"},    // Code icon
    {FileType::Header, "H"},    // Header icon
    {FileType::Image, "I"},     // Image icon
    {FileType::Audio, "A"},     // Audio icon
    {FileType::Video, "V"},     // Video icon
    {FileType::Document, "D"},  // Document icon
    {FileType::Archive, "Z"},   // Archive icon
    {FileType::Executable, "E"} // Executable icon
};

// Detect file type from extension
FileType DetectFileType(const std::wstring& filename) {
    // Extract extension
    size_t dotPos = filename.find_last_of(L'.');
    if (dotPos == std::wstring::npos) return FileType::Unknown;

    std::wstring extension = filename.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

    // Look up in the map
    auto it = extensionToFileType.find(extension);
    if (it != extensionToFileType.end()) {
        return it->second;
    }

    return FileType::Unknown;
}

// Check if a path is a directory
bool IsPathDirectory(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES &&
        (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

// Case-insensitive string contains
bool ContainsIgnoreCase(const std::string& str, const std::string& substr) {
    std::string lowerStr = str;
    std::string lowerSubstr = substr;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerSubstr.begin(), lowerSubstr.end(), lowerSubstr.begin(), ::tolower);
    return lowerStr.find(lowerSubstr) != std::string::npos;
}

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
            node.isBookmarked = IsBookmarked(fullPath);

            if (!isDirectory) {
                node.fileType = DetectFileType(filename);
            }

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

// Function to expand all folders
void ExpandAllFolders(std::vector<FileNode>& nodes) {
    for (auto& node : nodes) {
        if (node.isDirectory) {
            node.isOpen = true;
            ExpandAllFolders(node.children);
        }
    }
}

// Function to collapse all folders
void CollapseAllFolders(std::vector<FileNode>& nodes) {
    for (auto& node : nodes) {
        if (node.isDirectory) {
            node.isOpen = false;
            CollapseAllFolders(node.children);
        }
    }
}

// Add a bookmark
void AddBookmark(const std::wstring& path) {
    bookmarkedPaths.insert(path);

    // Update bookmarked state in hierarchy
    std::function<void(std::vector<FileNode>&)> updateBookmarks =
        [&path, &updateBookmarks](std::vector<FileNode>& nodes) {
        for (auto& node : nodes) {
            if (node.path == path) {
                node.isBookmarked = true;
            }
            if (node.isDirectory) {
                updateBookmarks(node.children);
            }
        }
        };

    updateBookmarks(hierarchyNodes);
}

// Remove a bookmark
void RemoveBookmark(const std::wstring& path) {
    bookmarkedPaths.erase(path);

    // Update bookmarked state in hierarchy
    std::function<void(std::vector<FileNode>&)> updateBookmarks =
        [&path, &updateBookmarks](std::vector<FileNode>& nodes) {
        for (auto& node : nodes) {
            if (node.path == path) {
                node.isBookmarked = false;
            }
            if (node.isDirectory) {
                updateBookmarks(node.children);
            }
        }
        };

    updateBookmarks(hierarchyNodes);
}

// Check if a path is bookmarked
bool IsBookmarked(const std::wstring& path) {
    return bookmarkedPaths.find(path) != bookmarkedPaths.end();
}

// Get all bookmarks
std::vector<std::wstring> GetBookmarks() {
    return std::vector<std::wstring>(bookmarkedPaths.begin(), bookmarkedPaths.end());
}

// Search in file names and content
std::vector<FileSearchResult> SearchInHierarchy(const std::string& searchTerm, bool matchCase, bool searchInContents) {
    std::vector<FileSearchResult> results;
    if (searchTerm.empty() || currentWorkingDirectory.empty()) return results;

    // Convert searchTerm for case-insensitive comparison if needed
    std::string searchTermProcessed = searchTerm;
    if (!matchCase) {
        std::transform(searchTermProcessed.begin(), searchTermProcessed.end(),
            searchTermProcessed.begin(), ::tolower);
    }

    // Recursive search function
    std::function<void(const std::wstring&)> searchInPath =
        [&](const std::wstring& path) {
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((path + L"\\*").c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring filename = findData.cFileName;
                if (filename == L"." || filename == L"..") continue;

                bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                std::wstring fullPath = path + L"\\" + filename;

                // Convert filename to string for comparison
                std::string filenameStr(filename.begin(), filename.end());
                std::string filenameProcessed = filenameStr;

                if (!matchCase) {
                    std::transform(filenameProcessed.begin(), filenameProcessed.end(),
                        filenameProcessed.begin(), ::tolower);
                }

                // Check filename match
                if (filenameProcessed.find(searchTermProcessed) != std::string::npos) {
                    FileSearchResult result;
                    result.path = fullPath;
                    result.name = filenameStr;
                    result.isDirectory = isDir;
                    results.push_back(result);
                }

                // Check file contents if needed
                if (searchInContents && !isDir) {
                    // Skip binary files or very large files
                    if (fullPath.find(L".exe") == std::wstring::npos &&
                        fullPath.find(L".dll") == std::wstring::npos) {

                        // Open and search file
                        std::ifstream file(fullPath);
                        if (file) {
                            std::string line;
                            int lineNum = 0;
                            bool found = false;
                            std::stringstream context;

                            while (std::getline(file, line) && !found) {
                                lineNum++;

                                std::string lineProcessed = line;
                                if (!matchCase) {
                                    std::transform(lineProcessed.begin(), lineProcessed.end(),
                                        lineProcessed.begin(), ::tolower);
                                }

                                if (lineProcessed.find(searchTermProcessed) != std::string::npos) {
                                    found = true;
                                    context << "Line " << lineNum << ": "
                                        << line.substr(0, 100)
                                        << (line.length() > 100 ? "..." : "");
                                }
                            }

                            if (found) {
                                FileSearchResult result;
                                result.path = fullPath;
                                result.name = filenameStr;
                                result.isDirectory = false;
                                result.matchContext = context.str();
                                results.push_back(result);
                            }
                        }
                    }
                }

                // Recurse into subdirectories
                if (isDir) {
                    searchInPath(fullPath);
                }

            } while (FindNextFileW(hFind, &findData) != 0);

            FindClose(hFind);
        }
        };

    // Start recursive search
    searchInPath(currentWorkingDirectory);
    return results;
}

// Handle drag and drop for node reordering or moving files
bool HandleDragDrop(FileNode& node) {
    bool modified = false;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        // Set payload to carry the node path
        ImGui::SetDragDropPayload("FILE_NODE", node.path.c_str(),
            (node.path.length() + 1) * sizeof(wchar_t));

        // Display preview while dragging
        ImGui::Text("Moving: %s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_NODE")) {
            // Only directories can accept drop
            if (node.isDirectory) {
                std::wstring srcPath = std::wstring((wchar_t*)payload->Data);
                std::wstring dstPath = node.path;

                // Get source filename
                size_t lastSlash = srcPath.find_last_of(L'\\');
                std::wstring srcFilename =
                    (lastSlash != std::wstring::npos) ? srcPath.substr(lastSlash + 1) : srcPath;

                // Create destination path
                std::wstring newPath = dstPath + L"\\" + srcFilename;

                // Don't move to itself
                if (srcPath != newPath) {
                    // Move file or directory
                    bool success = MoveFileW(srcPath.c_str(), newPath.c_str());
                    if (success) {
                        modified = true;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    return modified;
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

// Recursively render file nodes with enhanced features
void RenderFileNode(FileNode& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (!node.isDirectory) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // Add selected flag if this is a search result
    if (isSearchActive) {
        for (const auto& result : currentSearchResults) {
            if (result.path == node.path) {
                flags |= ImGuiTreeNodeFlags_Selected;
                break;
            }
        }
    }

    // Add star icon for bookmarked items
    std::string displayName;
    if (node.isBookmarked) {
        displayName = "* ";  // Star for bookmarks
    }
    else {
        displayName = "  ";
    }

    // Add file type icon
    if (node.isDirectory) {
        displayName += "[D] ";  // Folder icon
    }
    else {
        auto it = fileTypeToIcon.find(node.fileType);
        if (it != fileTypeToIcon.end()) {
            displayName += "[";
            displayName += it->second;
            displayName += "] ";
        }
        else {
            displayName += "[F] ";  // Default file icon
        }
    }

    // Add filename
    displayName += node.name;

    bool isOpen = ImGui::TreeNodeEx(displayName.c_str(), flags);

    // Remember open state
    node.isOpen = isOpen;

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

    // Handle drag and drop
    bool hierarchyModified = HandleDragDrop(node);
    if (hierarchyModified) {
        RefreshHierarchy();
    }

    // Enhanced context menu for files and folders
    if (ImGui::BeginPopupContextItem()) {
        // Open/Close folder options for directories
        if (node.isDirectory) {
            if (ImGui::MenuItem("Expand All")) {
                node.isOpen = true;
                ExpandAllFolders(node.children);
            }
            if (ImGui::MenuItem("Collapse All")) {
                CollapseAllFolders(node.children);
            }
            ImGui::Separator();
        }

        // Bookmark options
        if (node.isBookmarked) {
            if (ImGui::MenuItem("Remove Bookmark")) {
                RemoveBookmark(node.path);
            }
        }
        else {
            if (ImGui::MenuItem("Add Bookmark")) {
                AddBookmark(node.path);
            }
        }

        ImGui::Separator();

        // File operations
        if (ImGui::MenuItem("Delete")) {
            if (node.isDirectory) {
                // Delete directory (requires confirmation)
                if (MessageBox(NULL, L"Delete this folder and all its contents?",
                    L"Confirm Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
                    // Use SHFileOperation to delete folder with contents
                    SHFILEOPSTRUCTW fileOp = { 0 };
                    fileOp.wFunc = FO_DELETE;

                    // Need to double-null terminate the path
                    std::wstring pathWithNull = node.path + L'\0';
                    fileOp.pFrom = pathWithNull.c_str();

                    fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI;
                    int result = SHFileOperationW(&fileOp);
                    if (result == 0 && !fileOp.fAnyOperationsAborted) {
                        RefreshHierarchy();
                    }
                }
            }
            else {
                // Delete file (simpler)
                if (MessageBox(NULL, L"Delete this file?",
                    L"Confirm Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
                    if (DeleteFileW(node.path.c_str())) {
                        RefreshHierarchy();
                    }
                }
            }
        }

        if (node.isDirectory) {
            ImGui::Separator();

            if (ImGui::MenuItem("New File")) {
                // Will implement in a separate popup
                // For now, just a placeholder
                ImGui::OpenPopup("Create New File");
            }

            if (ImGui::MenuItem("New Folder")) {
                // Will implement in a separate popup
                // For now, just a placeholder
                ImGui::OpenPopup("Create New Folder");
            }
        }

        ImGui::EndPopup();
    }

    // New File dialog
    static char newFileName[256] = "newfile.txt";
    if (ImGui::BeginPopupModal("Create New File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter name for new file in:\n%s", node.name.c_str());
        ImGui::InputText("Filename", newFileName, sizeof(newFileName));

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            std::wstring newFilePath = node.path + L"\\" +
                std::wstring(newFileName, newFileName + strlen(newFileName));

            // Create empty file
            HANDLE hFile = CreateFileW(newFilePath.c_str(), GENERIC_WRITE, 0, NULL,
                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
                RefreshHierarchy();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // New Folder dialog
    static char newFolderName[256] = "New Folder";
    if (ImGui::BeginPopupModal("Create New Folder", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter name for new folder in:\n%s", node.name.c_str());
        ImGui::InputText("Folder Name", newFolderName, sizeof(newFolderName));

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            std::wstring newFolderPath = node.path + L"\\" +
                std::wstring(newFolderName, newFolderName + strlen(newFolderName));

            if (CreateDirectoryW(newFolderPath.c_str(), NULL)) {
                RefreshHierarchy();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
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

// Show the search popup when Ctrl+F is pressed
void ShowSearchPopup() {
    if (ImGui::BeginPopupModal("Search in Files", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char searchBuffer[256] = "";
        static bool matchCase = false;
        static bool searchContents = false;

        ImGui::Text("Search in current directory:");
        ImGui::InputText("##searchInput", searchBuffer, sizeof(searchBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Checkbox("Match Case", &matchCase);
        ImGui::SameLine();
        ImGui::Checkbox("Search in File Contents", &searchContents);

        if (ImGui::Button("Search", ImVec2(120, 0)) ||
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {

            // Perform search
            currentSearchTerm = searchBuffer;
            currentSearchResults = SearchInHierarchy(currentSearchTerm, matchCase, searchContents);
            isSearchActive = true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// Handle keyboard shortcut for search
bool HandleSearchShortcut() {
    ImGuiIO& io = ImGui::GetIO();
    bool ctrlPressed = io.KeyCtrl;
    bool fPressed = ImGui::IsKeyPressed(ImGuiKey_F);

    if (ctrlPressed && fPressed) {
        ImGui::OpenPopup("Search in Files");
        return true;
    }

    return false;
}

// Show the hierarchy window with enhanced features
void ShowHierarchyWindow() {
    ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse);

    // Handle keyboard shortcuts
    HandleSearchShortcut();

    // Search popup (triggered by Ctrl+F)
    ShowSearchPopup();

    // Top toolbar for hierarchy actions
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

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        RefreshHierarchy();
    }

    ImGui::SameLine();
    if (ImGui::Button("Expand All")) {
        ExpandAllFolders(hierarchyNodes);
    }

    ImGui::SameLine();
    if (ImGui::Button("Collapse All")) {
        CollapseAllFolders(hierarchyNodes);
    }

    // Clear search results button
    if (isSearchActive) {
        ImGui::SameLine();
        if (ImGui::Button("Clear Search")) {
            isSearchActive = false;
            currentSearchResults.clear();
        }
    }

    ImGui::Separator();

    // Bookmarks section
    if (ImGui::CollapsingHeader("Bookmarks")) {
        auto bookmarks = GetBookmarks();
        if (bookmarks.empty()) {
            ImGui::Text("No bookmarks yet");
        }
        else {
            for (const auto& bookmark : bookmarks) {
                // Extract just the folder/file name for display
                size_t lastSlash = bookmark.find_last_of(L'\\');
                std::wstring name =
                    (lastSlash != std::wstring::npos) ? bookmark.substr(lastSlash + 1) : bookmark;
                std::string nameStr(name.begin(), name.end());

                // Display as selectable item with icon
                std::string displayName = "* ";  // Star icon for bookmark

                // Add folder/file icon
                if (IsPathDirectory(bookmark)) {
                    displayName += "[D] ";
                }
                else {
                    displayName += "[F] ";
                }

                displayName += nameStr;

                if (ImGui::Selectable(displayName.c_str())) {
                    if (IsPathDirectory(bookmark)) {
                        SetWorkingDirectory(bookmark);
                    }
                    else {
                        // Open file if it's not a directory
                        std::string fileContent;
                        if (LoadTextFromFile(bookmark.c_str(), fileContent)) {
                            auto it = std::find_if(tabs.begin(), tabs.end(), [&bookmark](const Tab& tab) {
                                return tab.filename == bookmark;
                                });

                            if (it != tabs.end()) {
                                currentTabIndex = std::distance(tabs.begin(), it);
                            }
                            else {
                                tabs.push_back(Tab{ nameStr, fileContent, bookmark });
                                currentTabIndex = static_cast<int>(tabs.size()) - 1;
                            }
                        }
                    }
                }

                // Context menu for bookmark management
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Bookmark")) {
                        RemoveBookmark(bookmark);
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }

    ImGui::Separator();

    // Display search results if active
    if (isSearchActive && !currentSearchResults.empty()) {
        if (ImGui::CollapsingHeader("Search Results", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Found %d results for '%s'",
                (int)currentSearchResults.size(), currentSearchTerm.c_str());

            for (const auto& result : currentSearchResults) {
                // Create a descriptive display name
                std::string displayName = result.name;
                if (!result.matchContext.empty()) {
                    displayName += " - " + result.matchContext;
                }

                // Limit length for UI
                if (displayName.length() > 60) {
                    displayName = displayName.substr(0, 57) + "...";
                }

                // Add appropriate icon
                if (result.isDirectory) {
                    displayName = "[D] " + displayName;  // Directory
                }
                else {
                    FileType type = DetectFileType(result.path);
                    auto it = fileTypeToIcon.find(type);
                    if (it != fileTypeToIcon.end()) {
                        displayName = "[" + std::string(it->second) + "] " + displayName;
                    }
                    else {
                        displayName = "[F] " + displayName;  // Generic file
                    }
                }

                // Make it selectable to navigate to the item
                if (ImGui::Selectable(displayName.c_str())) {
                    if (result.isDirectory) {
                        // If directory, open it in hierarchy
                        SetWorkingDirectory(result.path);
                    }
                    else {
                        // If file, open it in editor
                        std::string fileContent;
                        if (LoadTextFromFile(result.path.c_str(), fileContent)) {
                            std::string name(result.name.begin(), result.name.end());

                            auto it = std::find_if(tabs.begin(), tabs.end(), [&result](const Tab& tab) {
                                return tab.filename == result.path;
                                });

                            if (it != tabs.end()) {
                                currentTabIndex = std::distance(tabs.begin(), it);
                            }
                            else {
                                tabs.push_back(Tab{ result.name, fileContent, result.path });
                                currentTabIndex = static_cast<int>(tabs.size()) - 1;
                            }
                        }
                    }
                }
            }
        }

        ImGui::Separator();
    }

    // Display current path and hierarchy
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