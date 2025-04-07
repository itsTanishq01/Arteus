#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include <windows.h>
#include <gl/gl.h>
#include <string>
#include "FileMenu.h"
#include "TextMenu.h"
#include "FileHierarchy.h"
#include "Terminal.h"
#include <algorithm>

struct WGL_WindowData { HDC hDC; };

static HGLRC g_hRC;
static WGL_WindowData g_MainWindow;
int g_Width;  
int g_Height; 

bool searchReplaceOpen = false;
std::string searchText;
std::string replaceText;
std::vector<SearchResult> searchResults;
bool caseSensitive = false;
size_t currentMatchIndex = 0;

bool fileBrowserOpen = false;
std::string selectedFilePath;

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**) {
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Arteus", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceWGL(hwnd, &g_MainWindow)) {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    tabs.push_back(Tab{ "Untitled", "" });
    currentTabIndex = 0;

    g_Terminal.Initialize();

    bool done = false;
    while (!done) {
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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (!currentWorkingDirectory.empty()) {
            g_Terminal.SetWorkingDirectory(currentWorkingDirectory);
        }

        // Calculate available space for other windows if terminal is open
        float terminalSpace = terminalWindowOpen ? terminalHeight : 0.0f;

        // Create the explorer window on the left with adjusted height
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(250, (float)g_Height - terminalSpace));
        ShowHierarchyWindow();

        // Create a text editor window with adjusted height
        ImGui::SetNextWindowPos(ImVec2(250, 0));
        ImGui::SetNextWindowSize(ImVec2((float)g_Width - 250, (float)g_Height - terminalSpace));
        ImGui::Begin("Text Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        ShowFileMenu(done);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Text")) {
                if (ImGui::MenuItem("Search & Replace")) {
                    searchReplaceOpen = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Terminal", NULL, &terminalWindowOpen);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open File Browser")) {
                    fileBrowserOpen = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        RenderTabs();

        if (currentTabIndex >= 0 && currentTabIndex < tabs.size()) {
            ShowSearchReplaceDialog(&searchReplaceOpen, searchText, replaceText, tabs[currentTabIndex].content,
                searchResults, caseSensitive, currentMatchIndex);
            ImGui::BeginChild("TextContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::EndChild();
        }

        if (currentTabIndex >= 0 && currentTabIndex < tabs.size()) {
            size_t totalCharacters = std::count_if(tabs[currentTabIndex].content.begin(), tabs[currentTabIndex].content.end(), [](char c) { return !std::isspace(c); });
            size_t totalLines = std::count(tabs[currentTabIndex].content.begin(), tabs[currentTabIndex].content.end(), '\n') + 1;
            ImGui::Separator();
            ImGui::Text("Total Characters: %zu | Total Lines: %zu |", totalCharacters, totalLines);
        }
        else {
            ImGui::Separator();
            ImGui::Text("No tabs open");
        }

        ImGui::End();

        // Render the terminal window at the bottom if it's open
        if (terminalWindowOpen) {
            g_Terminal.RenderWindow(&terminalWindowOpen);
        }

        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ::SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceWGL(hwnd, &g_MainWindow);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}