# Arteus Text Editor

This is a standalone example application using Dear ImGui for creating a simple text editor with OpenGL 3 and Win32. The application allows users to create, open, and save text files using a graphical user interface.

## Features

- **Text Editing**: Multiline text input with scrollbars.
- **File Operations**: Create a new file, open an existing file, and save the current text to a file.
- **Responsive UI**: Resizes with the window and supports basic keyboard controls.

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui): Immediate mode GUI library.
- OpenGL: Graphics API for rendering.
- Win32 API: Windows-specific functions for creating windows and handling messages.

## Compilation Instructions

### Requirements

- A C++ compiler that supports C++11 or later.
- OpenGL development libraries.
- Win32 SDK.

### Steps

1. Clone the repository or download the source code files.
2. Ensure you have all necessary dependencies installed (ImGui, OpenGL, etc.).
3. Include the ImGui headers and source files in your project.
4. Link against OpenGL and the necessary Windows libraries.
5. Compile and run the application.

### Example Build Command (using g++)

```bash
g++ -o ImGuiTextEditor main.cpp -lopengl32 -lgdi32 -luser32 -lcomdlg32
