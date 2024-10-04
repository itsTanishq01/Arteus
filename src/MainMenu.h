#ifndef MAINMENU_H
#define MAINMENU_H

#include <string>
#include <vector>
#include <windows.h>

// Structure to hold tab data
struct Tab {
    std::string title;
    std::string content;
};

extern std::vector<Tab> tabs;
extern int currentTabIndex;

bool LoadTextFromFile(const wchar_t* filename, std::string& text);
bool SaveTextToFile(const wchar_t* filename, const std::string& text);
bool OpenFileDialog(wchar_t* filename, DWORD maxFileNameLength);
bool SaveFileDialog(wchar_t* filename, DWORD maxFileNameLength);
void ShowMainMenu(bool& done);
void RenderTabs();

#endif // MAINMENU_H