#include "Terminal.h"
#include "imgui.h"
#include "FileHierarchy.h"
#include <algorithm>

// Define the global terminal instance
Terminal g_Terminal;

// Terminal window state
bool terminalWindowOpen = false;
float terminalHeight = 200.0f; // Default terminal height

Terminal::Terminal()
    : historyPos(-1),
    scrollToBottom(false),
    hChildStdIn_Rd(NULL),
    hChildStdIn_Wr(NULL),
    hChildStdOut_Rd(NULL),
    hChildStdOut_Wr(NULL),
    processActive(false)
{
    // Get current directory
    wchar_t buffer[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, buffer);
    workingDirectory = buffer;

    // Initialize process info
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
}

Terminal::~Terminal() {
    // Clean up process
    TerminateCurrentProcess();

    // Close pipes
    if (hChildStdIn_Rd) CloseHandle(hChildStdIn_Rd);
    if (hChildStdIn_Wr) CloseHandle(hChildStdIn_Wr);
    if (hChildStdOut_Rd) CloseHandle(hChildStdOut_Rd);
    if (hChildStdOut_Wr) CloseHandle(hChildStdOut_Wr);
}

bool Terminal::Initialize() {
    // Create pipes for redirecting stdin/stdout
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipe for stdin
    if (!CreatePipe(&hChildStdIn_Rd, &hChildStdIn_Wr, &saAttr, 0))
        return false;

    // Create pipe for stdout
    if (!CreatePipe(&hChildStdOut_Rd, &hChildStdOut_Wr, &saAttr, 0))
        return false;

    // Set handle inheritance
    if (!SetHandleInformation(hChildStdIn_Wr, HANDLE_FLAG_INHERIT, 0))
        return false;

    if (!SetHandleInformation(hChildStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
        return false;

    // Add welcome message
    outputLines.push_back("Arteus Terminal - Type commands here");
    outputLines.push_back("Type 'help' for a list of commands");
    outputLines.push_back("----------------------------------");

    return true;
}

void Terminal::ExecuteCommand(const std::string& command) {
    // Skip empty commands
    if (command.empty())
        return;

    // Add to history
    commandHistory.push_front(command);
    if (commandHistory.size() > MAX_HISTORY)
        commandHistory.pop_back();

    // Reset history navigation
    historyPos = -1;

    // Add command to output
    outputLines.push_back("> " + command);
    scrollToBottom = true;

    // Check for built-in commands
    if (command == "cls" || command == "clear") {
        Clear();
        return;
    }
    else if (command == "help") {
        outputLines.push_back("Available commands:");
        outputLines.push_back("  cls, clear - Clear the terminal");
        outputLines.push_back("  cd <dir>   - Change directory");
        outputLines.push_back("  help       - Show this help");
        scrollToBottom = true;
        return;
    }

    // Start cmd process
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hChildStdIn_Rd;
    si.hStdOutput = hChildStdOut_Wr;
    si.hStdError = hChildStdOut_Wr;

    // Build command line
    std::wstring wCommand = L"cmd.exe /c " + std::wstring(command.begin(), command.end());
    std::vector<wchar_t> cmdLine(wCommand.begin(), wCommand.end());
    cmdLine.push_back(0); // Null terminate

    // Create process
    if (!CreateProcessW(
        NULL,                              // No module name
        &cmdLine[0],                       // Command line
        NULL,                              // Process handle not inheritable
        NULL,                              // Thread handle not inheritable
        TRUE,                              // Set handle inheritance
        CREATE_NO_WINDOW,                  // Creation flags
        NULL,                              // Use parent's environment block
        workingDirectory.c_str(),          // Working directory
        &si,                               // Startup info
        &processInfo                       // Process information
    )) {
        outputLines.push_back("Error: Could not create process");
        scrollToBottom = true;
        return;
    }

    // Process created successfully
    processActive = true;

    // Read the output
    ReadOutput();

    // Check if command is 'cd' to update working directory
    if (command.substr(0, 3) == "cd ") {
        wchar_t newDir[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, newDir)) {
            workingDirectory = newDir;
        }
    }

    // Clean up process
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
    processActive = false;
}

void Terminal::ReadOutput() {
    const int bufferSize = 4096;
    char buffer[bufferSize];
    DWORD bytesRead;

    // Read from pipe until it's empty
    while (true) {
        // Check if there's any data to read
        DWORD available = 0;
        if (!PeekNamedPipe(hChildStdOut_Rd, NULL, 0, NULL, &available, NULL) || available == 0)
            break;

        // Read data
        if (!ReadFile(hChildStdOut_Rd, buffer, bufferSize - 1, &bytesRead, NULL) || bytesRead == 0)
            break;

        // Null terminate
        buffer[bytesRead] = '\0';

        // Process output line by line
        std::string output = buffer;
        size_t pos = 0;
        size_t lineEnd;

        while ((lineEnd = output.find('\n', pos)) != std::string::npos) {
            std::string line = output.substr(pos, lineEnd - pos);

            // Remove carriage returns
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            outputLines.push_back(line);
            pos = lineEnd + 1;
        }

        // Add the last line if any
        if (pos < output.length()) {
            std::string line = output.substr(pos);

            // Remove carriage returns
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            outputLines.push_back(line);
        }

        scrollToBottom = true;
    }
}

void Terminal::SetWorkingDirectory(const std::wstring& path) {
    workingDirectory = path;
}

std::wstring Terminal::GetWorkingDirectory() const {
    return workingDirectory;
}

void Terminal::Clear() {
    outputLines.clear();
    outputLines.push_back("Arteus Terminal");
    outputLines.push_back("----------------------------------");
    scrollToBottom = true;
}

void Terminal::TerminateCurrentProcess() {
    if (processActive && processInfo.hProcess) {
        TerminateProcess(processInfo.hProcess, 0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
        processActive = false;
    }
}

void Terminal::RenderWindow(bool* p_open) {
    extern int g_Width;
    extern int g_Height;

    // Position and size the terminal at the bottom of the screen
    ImGui::SetNextWindowPos(ImVec2(0, g_Height - terminalHeight));
    ImGui::SetNextWindowSize(ImVec2(g_Width, terminalHeight));

    // Set window flags
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoCollapse;

    // Start the terminal window
    if (!ImGui::Begin("Terminal", p_open, flags)) {
        ImGui::End();
        return;
    }

    // Handle resize from top edge
    float mouseY = ImGui::GetIO().MousePos.y;
    float windowY = g_Height - terminalHeight;

    if (mouseY >= windowY - 4 && mouseY <= windowY + 4) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (ImGui::IsMouseDown(0)) {
            terminalHeight = g_Height - mouseY;

            // Limit terminal size
            if (terminalHeight < 100) terminalHeight = 100;
            if (terminalHeight > g_Height * 0.7f) terminalHeight = g_Height * 0.7f;
        }
    }

    // Display current directory in the window
    std::string dirStr(workingDirectory.begin(), workingDirectory.end());

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Actions")) {
            if (ImGui::MenuItem("Clear Terminal")) {
                Clear();
            }
            if (ImGui::MenuItem("Kill Process", NULL, false, processActive)) {
                TerminateCurrentProcess();
            }
            ImGui::EndMenu();
        }

        ImGui::Text("Dir: %s", dirStr.c_str());

        ImGui::EndMenuBar();
    }

    // Calculate height for terminal output
    const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    // Display the terminal output
    ImGui::BeginChild("TerminalOutput", ImVec2(0, -footer_height), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Render output lines
    for (const auto& line : outputLines) {
        ImGui::TextUnformatted(line.c_str());
    }

    // Auto-scroll
    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    ImGui::EndChild();

    // Separator
    ImGui::Separator();

    // Terminal input
    static char inputBuf[1024] = "";
    ImGui::Text("> ");
    ImGui::SameLine();

    ImGui::PushItemWidth(-1);
    bool reclaim_focus = false;

    // Simple input text with Enter handling
    if (ImGui::InputText("##TerminalInput", inputBuf, sizeof(inputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue)) {

        std::string command = inputBuf;
        if (!command.empty()) {
            ExecuteCommand(command);
            inputBuf[0] = '\0';
            reclaim_focus = true;
        }
    }

    // Manual history navigation with arrow keys
    if (ImGui::IsItemFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (historyPos < (int)commandHistory.size() - 1) {
                historyPos++;
                strcpy_s(inputBuf, commandHistory[historyPos].c_str());
            }
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (historyPos > -1) {
                historyPos--;
                if (historyPos >= 0) {
                    strcpy_s(inputBuf, commandHistory[historyPos].c_str());
                }
                else {
                    inputBuf[0] = '\0';
                }
            }
        }
    }

    // Auto-focus on the input box when window is appearing
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus) {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::PopItemWidth();
    ImGui::End();
}