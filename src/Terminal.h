#ifndef TERMINAL_H
#define TERMINAL_H

#include <string>
#include <vector>
#include <deque>
#include <Windows.h>

// Maximum number of history entries to keep
const int MAX_HISTORY = 50;

// Terminal height (export as extern so Main.cpp can use it)
extern float terminalHeight;

class Terminal {
public:
    Terminal();
    ~Terminal();

    // Initialize the terminal
    bool Initialize();

    // Execute a command
    void ExecuteCommand(const std::string& command);

    // Set the working directory
    void SetWorkingDirectory(const std::wstring& path);

    // Get the working directory
    std::wstring GetWorkingDirectory() const;

    // Clear the terminal output
    void Clear();

    // Render the terminal window
    void RenderWindow(bool* p_open);

private:
    // Terminal state
    std::vector<std::string> outputLines;
    std::deque<std::string> commandHistory;
    std::string inputBuffer;
    std::wstring workingDirectory;
    int historyPos;
    bool scrollToBottom;

    // Command execution
    PROCESS_INFORMATION processInfo;
    HANDLE hChildStdIn_Rd;
    HANDLE hChildStdIn_Wr;
    HANDLE hChildStdOut_Rd;
    HANDLE hChildStdOut_Wr;
    bool processActive;

    // Read output from the command
    void ReadOutput();

    // Terminate the current process
    void TerminateCurrentProcess();
};

// Global terminal instance
extern Terminal g_Terminal;

// Terminal window state
extern bool terminalWindowOpen;

#endif // TERMINAL_H