# STL Viewer

A lightweight desktop application for viewing binary STL 3D models, built with modern C++ and OpenGL.

**[Download the latest version](https://github.com/archuser-q/stl-viewer/releases/latest)** for Windows or Linux.

## Features

- Open binary STL files from the File menu or the command line
- Rotate, pan, and zoom with the mouse
- Automatically center and fit models to the view
- Render models with OpenGL shaders and directional lighting
- Show clear error messages for invalid files
- Build for Windows and Linux with GitHub Actions

## Controls

| Action | Input |
| --- | --- |
| Rotate | Hold left mouse button and drag |
| Pan | Hold middle mouse button and drag |
| Zoom | Mouse wheel |
| Open file | `Ctrl+O` |
| Quit | `Ctrl+Q` |

## Tech Stack

- C++17
- OpenGL 3.3 Core
- GLFW
- GLM
- Dear ImGui
- Native File Dialog Extended
- CMake
- GitHub Actions

## Run

### Windows

Download `stl-viewer.exe` from the [Releases page](https://github.com/archuser-q/stl-viewer/releases/latest) and run it.

### Linux

Download the AppImage from the [Releases page](https://github.com/archuser-q/stl-viewer/releases/latest), then run:

```bash
chmod +x STL_Viewer-x86_64.AppImage
./STL_Viewer-x86_64.AppImage
```

## Build from Source

### Ubuntu

Install the dependencies:

```bash
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev \
  libwayland-dev libxkbcommon-dev libgtk-3-dev
```

Build and run:

```bash
git clone https://github.com/archuser-q/stl-viewer.git
cd stl-viewer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/stl-viewer path/to/model.stl
```

### Windows

Requires Visual Studio with C++ tools, CMake, and Git.

```powershell
git clone https://github.com/archuser-q/stl-viewer.git
cd stl-viewer
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\stl-viewer.exe
```

## Project Structure

```text
.
├── .github/workflows/   # Windows and AppImage builds
├── src/
│   ├── main.cpp         # Window, UI, input, and rendering
│   ├── stl_loader.cpp   # Binary STL parser
│   └── stl_loader.h     # Mesh data structures
├── CMakeLists.txt
└── icon.png
```

## Limitations

- ASCII STL files are not supported yet.