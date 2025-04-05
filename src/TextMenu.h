#ifndef TEXTMENU_H
#define TEXTMENU_H

#include <string>
#include <vector>

struct SearchResult {
    size_t start;
    size_t end;
};

// Function to display the search and replace dialog
void ShowSearchReplaceDialog(bool* open, std::string& searchText, std::string& replaceText,
    std::string& text, std::vector<SearchResult>& searchResults,
    bool& caseSensitive, size_t& currentMatchIndex); 

#endif // TEXTMENU_H
