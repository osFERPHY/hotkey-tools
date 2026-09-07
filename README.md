# ⌨️ Hotkey_Tools

A lightweight desktop application built with **C++**, **Dear ImGui**, and the **Windows API**, designed for managing global hotkeys, quick app launching, and casually trolling friends during screen shares (like popping up random images or borderless videos).

> ⚠️ **Developer's Note:** Yes, some of the GUI boilerplate was generated with AI. I still had to integrate it, debug it, and make the whole thing actually work. 😄

## 🚀 Features

- **Global Keybinds:** Map keys to specific actions and trigger them globally using low-level Windows API checks (`GetAsyncKeyState`).
- **Media & App Launcher:** Instantly open URLs, launch `.exe` files, or render images and videos.
- **Borderless Fullscreen (Troll Mode):** Render images (via GDI+) and videos (via MCI) borderless and fullscreen over all other windows. Perfect for jumpscares, memes, or quick references.
- **Drag & Drop Integration:** Drop files directly into the window — the tool automatically parses and copies them to the local directory.
- **Dynamic File Scanning:** Drop files into the local `zalejnost/` folder and hit `update` — the tool automatically parses new media without restarting.
- **System Tray Integration:** Hides neatly in the Windows background tray to keep your taskbar clean. Right-click to open or exit.
- **Auto-Start Support:** Can be configured to automatically launch minimized with Windows.

## 🛠 Tech Stack

- **Language:** C++17
- **GUI:** Dear ImGui (GLFW + OpenGL2 backend)
- **OS Interfacing:** Windows API (WinAPI, GDI+, ShellAPI, winmm)
- **Build System:** CMake
- **Data Storage:** Lightweight local `.txt` files for lists and configurations.

## ⚙️ How to Build

Clone the repository and build it using CMake:

```bash
git clone https://github.com/osFERPHY/Hotkey_Tolips.git
cd Hotkey_Tolips
mkdir build
cd build
cmake ..
cmake --build . --config Release
