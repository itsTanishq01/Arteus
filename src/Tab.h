#ifndef TAB_H
#define TAB_H

#include <string>

struct Tab {
    std::string title;
    std::string content;
    std::wstring filename; // Filename as std::wstring

    // Updated constructor to accept title, content, and filename
    Tab(const std::string& title, const std::string& content, const std::wstring& filename = L"")
        : title(title), content(content), filename(filename) {}
};

#endif // TAB_H
