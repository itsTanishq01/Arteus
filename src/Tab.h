#ifndef TAB_H
#define TAB_H

#include <string>

struct Tab {
    std::string title;
    std::string content;
    std::wstring filename; // Filename as std::wstring

    Tab(const std::string& title, const std::string& content)
        : title(title), content(content) {}
};

#endif // TAB_H
