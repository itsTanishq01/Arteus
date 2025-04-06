#ifndef FILEHIERARCHY_H
#define FILEHIERARCHY_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

// Define file types for icon display
enum class FileType {
    Unknown,
    Text,
    Source,
    Header,
    Image,
    Audio,
    Video,
    Document,
    Archive,
    Executable
};

// Structure to represent a file or directory in the hierarchy
struct FileNode {
    std::string name;
    std::wstring path;
    bool isDirectory;
    bool isOpen;
    bool isBookmarked;
    FileType fileType;
    std::vector<FileNode> children;

    // Constructor with default values
    FileNode() : isDirectory(false), isOpen(false), isBookmarked(false), fileType(FileType::Unknown) {}
};

// Structure to store file hierarchy search results
struct FileSearchResult {
    std::wstring path;
    std::string name;
    bool isDirectory;
    std::string matchContext;  // For content search, shows context around match
};

// Function to display the hierarchy window
void ShowHierarchyWindow();

// Function to build the file hierarchy from a root directory
void BuildFileHierarchy(const std::wstring& rootPath, std::vector<FileNode>& hierarchy);

// Function to refresh the hierarchy
void RefreshHierarchy();

// Function to set the current working directory
void SetWorkingDirectory(const std::wstring& path);

// Function to detect file type from extension
FileType DetectFileType(const std::wstring& filename);

// Functions to expand/collapse folders
void ExpandAllFolders(std::vector<FileNode>& nodes);
void CollapseAllFolders(std::vector<FileNode>& nodes);

// Function to search in file hierarchy
std::vector<FileSearchResult> SearchInHierarchy(const std::string& searchTerm, bool matchCase, bool searchInContents);

// Bookmark management functions
void AddBookmark(const std::wstring& path);
void RemoveBookmark(const std::wstring& path);
bool IsBookmarked(const std::wstring& path);
std::vector<std::wstring> GetBookmarks();

// Drag and drop support
bool HandleDragDrop(FileNode& node);

// Utility function to check if path is a directory
bool IsPathDirectory(const std::wstring& path);

// Current working directory
extern std::wstring currentWorkingDirectory;

// Root hierarchy nodes
extern std::vector<FileNode> hierarchyNodes;

// Bookmarked paths
extern std::unordered_set<std::wstring> bookmarkedPaths;

// File search state
extern bool isSearchActive;
extern std::vector<FileSearchResult> currentSearchResults;
extern std::string currentSearchTerm;

// File extension to FileType mapping
extern std::unordered_map<std::wstring, FileType> extensionToFileType;

#endif // FILEHIERARCHY_H