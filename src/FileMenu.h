<<<<<<< HEAD
#ifndef FILEMENU_H
#define FILEMENU_H

#include <vector>
#include <windows.h>
#include "Tab.h" // Include the new Tab header
=======
// FileMenu.h
#ifndef FILEMENU_H
#define FILEMENU_H

#include <string>
#include <vector>
#include <windows.h>

// Structure to hold tab data
struct Tab {
    std::string title;
    std::string content;
    std::wstring filename; // Filename as std::wstring
};
>>>>>>> 4c9d8f9a5ad35f31829ee7f7409a86e043672e66

extern std::vector<Tab> tabs; // Collection of tabs
extern int currentTabIndex; // Index of the currently active tab

bool LoadTextFromFile(const wchar_t* filename, std::string& text);
bool SaveTextToFile(const wchar_t* filename, const std::string& text);
bool OpenFileDialog(wchar_t* filename, DWORD maxFileNameLength);
bool SaveFileDialog(wchar_t* filename, DWORD maxFileNameLength);
void ShowFileMenu(bool& done);
void RenderTabs();

#endif // FILEMENU_H
