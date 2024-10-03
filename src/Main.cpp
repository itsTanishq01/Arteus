#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>
#include <fstream>
#include <string>
#include <vector>
#include <commdlg.h>
#include <algorithm>
#include <sstream>

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC g_hRC;
static WGL_WindowData g_MainWindow;
static int g_Width;
static int g_Height;

// Structure to hold tab data
struct Tab {
    std::string title;
    std::string content;
};

std::vector<Tab> tabs; // Collection of tabs
int currentTabIndex = -1; // Index of the currently active tab

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Function to load text from a file
bool LoadTextFromFile(const wchar_t* filename, std::string& text) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}

// Function to save text to a file
bool SaveTextToFile(const wchar_t* filename, const std::string& text) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << text;
    return true;
}

// Function to open a file dialog and get the selected file path
bool OpenFileDialog(wchar_t* filename, DWORD maxFileNameLength) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = maxFileNameLength;
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameW(&ofn);
}

// Function to save a file dialog and get the selected file path
bool SaveFileDialog(wchar_t* filename, DWORD maxFileNameLength) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = maxFileNameLength;
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    return GetSaveFileNameW(&ofn);
}

// Main code
int main(int, char**) {
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Arteus", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow)) {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Initialize the first tab
    tabs.push_back(Tab{ "Untitled", "" });
    currentTabIndex = 0;

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd)) {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Create a text editor window
        ImGui::Begin("Text Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2((float)g_Width, (float)g_Height), ImGuiCond_Always);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    tabs.push_back(Tab{ "Untitled", "" });
                    currentTabIndex = tabs.size() - 1; // Switch to the new tab
                }
                if (ImGui::MenuItem("Open")) {
                    wchar_t filename[256] = L"";
                    if (OpenFileDialog(filename, sizeof(filename))) {
                        std::string file_text;
                        if (LoadTextFromFile(filename, file_text)) {
                            // Convert wchar_t filename to std::string
                            std::wstring wFileName = filename; // Convert to std::wstring for better readability
                            std::string fullPath(wFileName.begin(), wFileName.end()); // Convert std::wstring to std::string

                            // Extract the file name from the full path
                            std::string fileName = fullPath.substr(fullPath.find_last_of("/\\") + 1);

                            tabs.push_back(Tab{ fileName, file_text }); // Add new tab with extracted filename and content
                            currentTabIndex = tabs.size() - 1; // Switch to the new tab
                        }
                        else {
                            ImGui::Text("Error loading file!");
                        }
                    }
                }
                if (ImGui::MenuItem("Save")) {
                    if (currentTabIndex >= 0 && currentTabIndex < tabs.size()) {
                        wchar_t filename[256] = L"";
                        if (SaveFileDialog(filename, sizeof(filename))) {
                            SaveTextToFile(filename, tabs[currentTabIndex].content);
                        }
                    }
                }
                if (ImGui::MenuItem("Exit")) {
                    done = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Text")) {
                if (ImGui::MenuItem("Search & Replace")) {
                    // Implement search and replace dialog here
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Create the tab bar
        if (ImGui::BeginTabBar("Tabs")) {
            for (int i = 0; i < tabs.size(); i++) {
                const bool isSelected = (currentTabIndex == i);
                if (ImGui::BeginTabItem(tabs[i].title.c_str(), nullptr, isSelected ? ImGuiTabItemFlags_SetSelected : 0)) {
                    if (isSelected) {
                        ImGui::InputTextMultiline("##text", &tabs[i].content[0], tabs[i].content.size() + 1, ImVec2(-FLT_MIN, -FLT_MIN),
                            ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
                            [](ImGuiInputTextCallbackData* data) {
                                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                                    std::string* text = (std::string*)data->UserData;
                                    text->resize(data->BufTextLen); // Resize the string to the new length
                                    data->Buf = &((*text)[0]); // Update the buffer pointer
                                }
                                return 0;
                            }, (void*)&tabs[i].content);
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        // Status bar for current tab
        if (currentTabIndex >= 0 && currentTabIndex < tabs.size()) {
            size_t totalCharacters = std::count_if(tabs[currentTabIndex].content.begin(), tabs[currentTabIndex].content.end(), [](char c) { return !std::isspace(c); });
            size_t totalLines = std::count(tabs[currentTabIndex].content.begin(), tabs[currentTabIndex].content.end(), '\n') + 1;
            ImGui::Separator();
            ImGui::Text("Total Characters: %zu | Total Lines: %zu", totalCharacters, totalLines);
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data) {
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data) {
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
