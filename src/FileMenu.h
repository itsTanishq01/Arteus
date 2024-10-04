#ifndef FILEMENU_H
#define FILEMENU_H

#include <vector>
#include <windows.h>
#include "Tab.h" // Include the new Tab header

extern std::vector<Tab> tabs; // Collection of tabs
extern int currentTabIndex; // Index of the currently active tab

bool LoadTextFromFile(const wchar_t* filename, std::string& text);
bool SaveTextToFile(const wchar_t* filename, const std::string& text);
bool OpenFileDialog(wchar_t* filename, DWORD maxFileNameLength);
bool SaveFileDialog(wchar_t* filename, DWORD maxFileNameLength);
void ShowFileMenu(bool& done);
void RenderTabs();

#endif // FILEMENU_H
