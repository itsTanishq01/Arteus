#ifndef FILEHIERARCHY_H
#define FILEHIERARCHY_H

#include <string>
#include <vector>
#include <filesystem>
#include <functional>

// Structure to represent a file or directory in the hierarchy
struct FileNode {
    std::string name;
    std::wstring path;
    bool isDirectory;
    bool isOpen;
    std::vector<FileNode> children;
};

// Function to display the hierarchy window
void ShowHierarchyWindow();

// Function to build the file hierarchy from a root directory
void BuildFileHierarchy(const std::wstring& rootPath, std::vector<FileNode>& hierarchy);

// Function to refresh the hierarchy
void RefreshHierarchy();

// Function to set the current working directory
void SetWorkingDirectory(const std::wstring& path);

// Current working directory
extern std::wstring currentWorkingDirectory;

// Root hierarchy nodes
extern std::vector<FileNode> hierarchyNodes;

#endif // FILEHIERARCHY_H
