#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <shellapi.h>
#include <filesystem>
#include <algorithm>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")



namespace fs = std::filesystem;



namespace actions {

    using namespace Gdiplus;

    // ==================== ШЛЯХИ ====================

    inline std::string GetExeDir() {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string path(exePath);
        size_t pos = path.find_last_of("\\/");
        return (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
    }

    inline std::string GetZalejnostDir() {
        std::string dir = GetExeDir() + "zalejnost\\";
        CreateDirectoryA(dir.c_str(), nullptr); // якщо вже існує - просто нічого не робить
        return dir;
    }

    // ==================== СПИСКИ (порожні на старті, читаються з файлів) ====================

    inline std::vector<std::string> program_paths;
    inline std::vector<std::string> link_list;
    inline std::vector<std::string> video_paths;
    inline std::vector<std::string> image_paths;

    inline void SaveListToFile(const std::string& filename, const std::vector<std::string>& list) {
        std::ofstream f(GetExeDir() + filename);
        for (auto& item : list) f << item << "\n";
    }

    inline void LoadListFromFile(const std::string& filename, std::vector<std::string>& list) {
        list.clear();
        std::ifstream f(GetExeDir() + filename);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) list.push_back(line);
        }
    }

    inline void LoadAllLists() {
        LoadListFromFile("programs.txt", program_paths);
        LoadListFromFile("links.txt", link_list);
        LoadListFromFile("videos.txt", video_paths);
        LoadListFromFile("images.txt", image_paths);
    }

    // ==================== ЗВУК ====================

    inline void PlaySound(int freq) {
        Beep(freq, 200);
        Sleep(250);
    }

    // ==================== ПРОГРАМИ ====================

    inline void LaunchProgram(const std::string& path) {
        ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // ==================== ПОСИЛАННЯ ====================

    inline void OpenLink(const std::string& url) {
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // ==================== ВІДЕО (фулскрін без рамок) ====================

    inline bool video_is_playing = false;
    inline HWND video_hwnd = nullptr;

    inline LRESULT CALLBACK VideoWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
            mciSendStringA("close myvideo", nullptr, 0, nullptr);
            video_is_playing = false;
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    inline HWND CreateVideoWindow() {
        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSA wc = {};
            wc.lpfnWndProc = VideoWndProc;
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpszClassName = "VideoPlayerWndClass";
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            RegisterClassA(&wc);
            classRegistered = true;
        }
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        return CreateWindowExA(WS_EX_TOPMOST, "VideoPlayerWndClass", "Video", WS_POPUP,
            0, 0, screenW, screenH, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    }

    inline void StopVideo() {
        if (video_is_playing) {
            mciSendStringA("close myvideo", nullptr, 0, nullptr);
            video_is_playing = false;
        }
        if (video_hwnd) ShowWindow(video_hwnd, SW_HIDE);
    }

    inline void PlayVideoPath(const std::string& path) {
        StopVideo();
        if (!video_hwnd) video_hwnd = CreateVideoWindow();

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        char parentArg[32];
        sprintf_s(parentArg, "%lld", (long long)(intptr_t)video_hwnd);

        std::string open_cmd = "open \"" + path + "\" alias myvideo style 1073741824 parent " + parentArg;

        if (mciSendStringA(open_cmd.c_str(), nullptr, 0, nullptr) == 0) {
            char putCmd[128];
            sprintf_s(putCmd, "put myvideo window at 0 0 %d %d", screenW, screenH);
            mciSendStringA(putCmd, nullptr, 0, nullptr);
            ShowWindow(video_hwnd, SW_SHOW);
            SetForegroundWindow(video_hwnd);
            mciSendStringA("play myvideo", nullptr, 0, nullptr);
            video_is_playing = true;
        }
    }

    // ==================== ФОТО (фулскрін через GDI+) ====================

    inline ULONG_PTR gdiplusToken = 0;
    inline bool image_is_showing = false;
    inline HWND image_hwnd = nullptr;
    inline Bitmap* current_image = nullptr;

    inline void InitGdiplus() {
        static bool initialized = false;
        if (!initialized) {
            GdiplusStartupInput input;
            GdiplusStartup(&gdiplusToken, &input, nullptr);
            initialized = true;
        }
    }

    inline LRESULT CALLBACK ImageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
            ShowWindow(hwnd, SW_HIDE);
            image_is_showing = false;
            return 0;
        }
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (current_image) {
                Graphics graphics(hdc);
                RECT rc;
                GetClientRect(hwnd, &rc);
                graphics.DrawImage(current_image, 0, 0, rc.right, rc.bottom);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    inline HWND CreateImageWindow() {
        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSA wc = {};
            wc.lpfnWndProc = ImageWndProc;
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpszClassName = "ImageViewerWndClass";
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            RegisterClassA(&wc);
            classRegistered = true;
        }
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        return CreateWindowExA(WS_EX_TOPMOST, "ImageViewerWndClass", "Image", WS_POPUP,
            0, 0, screenW, screenH, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    }

    inline void StopImage() {
        if (image_hwnd) ShowWindow(image_hwnd, SW_HIDE);
        image_is_showing = false;
    }

    inline void ShowImagePath(const std::string& path) {
        InitGdiplus();
        if (!image_hwnd) image_hwnd = CreateImageWindow();

        delete current_image;
        std::wstring wpath(path.begin(), path.end()); // працює для шляхів без кирилиці
        current_image = new Bitmap(wpath.c_str());

        ShowWindow(image_hwnd, SW_SHOW);
        SetForegroundWindow(image_hwnd);
        InvalidateRect(image_hwnd, nullptr, TRUE);
        image_is_showing = true;
    }



    // ==================== short http ====================
    inline std::string GetFileName(const std::string& path) {
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return path.substr(pos + 1);
        }
        return path;
    }
    // ==================== СПИСОК ДІЙ ====================

    struct ActionEntry {
        std::string name;
        std::function<void()> func;
    };

    inline std::vector<ActionEntry> actions_list;
    inline std::vector<const char*> list_storage;
    inline const char** list = nullptr;
    inline int count = 0;

    inline void RebuildActionsList() {
        actions_list.clear();

        actions_list.push_back({ "Вимкнено",     []() {} });


        // Одна кнопка, яка зупиняє і відео, і фото одночасно
        actions_list.push_back({ "Відео: СТОП",  []() {
            StopVideo();
            StopImage();
        } });

        // Рядок: actions_list.push_back({ "Фото: СТОП", []() { StopImage(); } }); -- ВИДАЛЯЄМО

        // Програми (показуємо тільки назву .exe)
        for (auto& path : program_paths) {
            actions_list.push_back({ "Запуск: " + GetFileName(path), [path]() { LaunchProgram(path); } });
        }

        // Посилання
        for (auto& url : link_list) {
            actions_list.push_back({ "Посилання: " + url, [url]() { OpenLink(url); } });
        }

        // Відео
        for (auto& path : video_paths) {
            actions_list.push_back({ "Відео: " + GetFileName(path), [path]() { PlayVideoPath(path); } });
        }

        // Фото
        for (auto& path : image_paths) {
            actions_list.push_back({ "Фото: " + GetFileName(path), [path]() { ShowImagePath(path); } });
        }

        list_storage.clear();
        for (auto& a : actions_list) list_storage.push_back(a.name.c_str());
        list = list_storage.data();
        count = (int)actions_list.size();
    }



inline void ScanZalejnostFolder() {
    std::string dirPath = GetZalejnostDir();
    if (!fs::exists(dirPath)) return;

    // 1. Спочатку завантажуємо актуальні списки з txt-файлів
    LoadAllLists();

    // 2. Видаляємо з пам'яті ті файли з папки zalejnost, які були вилучені або перейменовані
    auto cleanupMissingFiles = [&dirPath](std::vector<std::string>& vec) {
        vec.erase(std::remove_if(vec.begin(), vec.end(), [&dirPath](const std::string& path) {
            // Перевіряємо тільки файли, які лежать у папці zalejnost
            if (path.find(dirPath) == 0) {
                return !fs::exists(path); // Видаляємо зі списку, якщо файла більше немає
            }
            return false;
        }), vec.end());
    };

    cleanupMissingFiles(program_paths);
    cleanupMissingFiles(video_paths);
    cleanupMissingFiles(image_paths);

    // 3. Скануємо папку zalejnost на наявність нових файлів
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string ext = entry.path().extension().string();

            // Переводимо розширення у нижній регістр (.MP4 -> .mp4)
            for (auto& c : ext) c = (char)tolower(c);

            auto addIfMissing = [](std::vector<std::string>& vec, const std::string& path) {
                if (std::find(vec.begin(), vec.end(), path) == vec.end()) {
                    vec.push_back(path);
                }
            };

            if (ext == ".exe") {
                addIfMissing(program_paths, filePath);
            } else if (ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".wmv") {
                addIfMissing(video_paths, filePath);
            } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif") {
                addIfMissing(image_paths, filePath);
            }
        }
    }

    // 4. Зберігаємо оновлені списки у txt-файли
    SaveListToFile("programs.txt", program_paths);
    SaveListToFile("links.txt", link_list);
    SaveListToFile("videos.txt", video_paths);
    SaveListToFile("images.txt", image_paths);

    // 5. Перебудовуємо випадаючі списки для ImGui
    RebuildActionsList();
}

    inline void Execute(int action_id) {
        if (action_id >= 0 && action_id < (int)actions_list.size()) {
            actions_list[action_id].func();
        }
    }
}