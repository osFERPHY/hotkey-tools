#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

// бібліотеки hpp
#include "settigs.hpp"
#include "hotkeys.hpp"
#include "actions.hpp"

#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#define ID_TRAY_OPEN  1001
#define ID_TRAY_EXIT  1002
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// НОВІ ІНКЛУДИ ДЛЯ ТРЕЮ
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATA nid = {};
WNDPROC OriginalWndProc;

// Ця функція ловить клік по іконці в треї і розгортає вікно
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_TRAYICON) {
        GLFWwindow* window = (GLFWwindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (LOWORD(lParam) == WM_LBUTTONUP) { // Клік лівою кнопкою — розгортаємо вікно
            glfwShowWindow(window);
            glfwRestoreWindow(window);
        }
        else if (LOWORD(lParam) == WM_RBUTTONUP) { // Клік правою кнопкою — показуємо меню
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"Відкрити");
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Вийти");

            SetForegroundWindow(hwnd);

            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
    }
    else if (uMsg == WM_COMMAND) {
        int wmId = LOWORD(wParam);
        GLFWwindow* window = (GLFWwindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (wmId == ID_TRAY_OPEN) {
            glfwShowWindow(window);
            glfwRestoreWindow(window);
        }
        else if (wmId == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    return CallWindowProc(OriginalWndProc, hwnd, uMsg, wParam, lParam);
}

// Функція для перехоплення натискань клавіш через GLFW
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (settigs::listening_index != -1 && action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            settigs::bind_items[settigs::listening_index].key = 0;
        } else {
            settigs::bind_items[settigs::listening_index].key = key;
        }

        settigs::save();
        settigs::listening_index = -1;
    }
}

// Допоміжна функція для перетворення коду клавіші у текст
inline std::string GetKeyName(int key) {
    if (key == 0) return "Немає";
    if (key == GLFW_KEY_SPACE) return "SPACE";
    if (key == GLFW_KEY_ESCAPE) return "ESC";
    if (key == GLFW_KEY_ENTER) return "ENTER";
    if (key == GLFW_KEY_TAB) return "TAB";
    if (key == GLFW_KEY_BACKSPACE) return "BACKSPACE";
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) return "SHIFT";
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) return "CTRL";
    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) return "ALT";

    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
        return "NUM " + std::to_string(key - GLFW_KEY_KP_0);
    }
    if (key == GLFW_KEY_KP_DECIMAL) return "NUM .";
    if (key == GLFW_KEY_KP_DIVIDE) return "NUM /";
    if (key == GLFW_KEY_KP_MULTIPLY) return "NUM *";
    if (key == GLFW_KEY_KP_SUBTRACT) return "NUM -";
    if (key == GLFW_KEY_KP_ADD) return "NUM +";
    if (key == GLFW_KEY_KP_ENTER) return "NUM ENTER";

    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
        char buf[2] = { (char)key, '\0' };
        return std::string(buf);
    }

    const char* name = glfwGetKeyName(key, 0);
    if (name != nullptr) {
        std::string s(name);
        for (auto& c : s) c = toupper(c);
        return s;
    }

    return std::to_string(key);
}

static void drop_callback(GLFWwindow* window, int count, const char** paths) {
    std::string zalDir = actions::GetZalejnostDir();

    for (int i = 0; i < count; i++) {
        std::string srcPath = paths[i];
        size_t slashPos = srcPath.find_last_of("\\/");
        std::string filename = (slashPos != std::string::npos) ? srcPath.substr(slashPos + 1) : srcPath;
        std::string dstPath = zalDir + filename;

        CopyFileA(srcPath.c_str(), dstPath.c_str(), FALSE);

        std::string ext;
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            ext = filename.substr(dotPos);
            for (auto& c : ext) c = (char)tolower(c);
        }

        if (ext == ".exe") {
            actions::program_paths.push_back(dstPath);
            actions::SaveListToFile("programs.txt", actions::program_paths);
        } else if (ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".wmv") {
            actions::video_paths.push_back(dstPath);
            actions::SaveListToFile("videos.txt", actions::video_paths);
        } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif") {
            actions::image_paths.push_back(dstPath);
            actions::SaveListToFile("images.txt", actions::image_paths);
        }
    }

    actions::RebuildActionsList();
}

static void window_close_callback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    glfwHideWindow(window);
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int argc, char* argv[])
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    GLFWwindow* window = glfwCreateWindow((int)(400 * main_scale), (int)(500 * main_scale), "HootKey Toolips", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // -------------------------------------------------------------
    // ІКОНКА В ТРЕЇ ТА ПРИХОВУВАННЯ
    // -------------------------------------------------------------
    HWND hwnd = glfwGetWin32Window(window);
    HICON hCustomIcon = (HICON)LoadImageA(
         NULL,
         "avtar.ico",
         IMAGE_ICON,
         0, 0,
         LR_LOADFROMFILE | LR_DEFAULTSIZE
     );

    if (!hCustomIcon) {
        hCustomIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hCustomIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hCustomIcon);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hCustomIcon;
    strcpy_s(nid.szTip, "Hotkey Tool");
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    OriginalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

    // -------------------------------------------------------------
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);
    glfwSetDropCallback(window, drop_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // -------------------------------------------------------------
    // ДОДАВАННЯ КИРИЛИЦІ
    // -------------------------------------------------------------
    const ImWchar* cyrillic_ranges = io.Fonts->GetGlyphRangesCyrillic();
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\arial.ttf",
        18.0f,
        nullptr,
        cyrillic_ranges
    );

    // -------------------------------------------------------------
    // НАЛАШТУВАННЯ СТИЛЮ ТА КОЛЬОРІВ IMGUI
    // -------------------------------------------------------------
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Зкруглення елементів
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.TabRounding   = 6.0f;

    // Налаштування кольорів вкладок (TabBar / TabItem)
    style.Colors[ImGuiCol_Tab]                = ImVec4(0.15f, 0.15f, 0.18f, 1.0f); // Неактивна вкладка
    style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.35f, 0.55f, 0.85f, 1.0f); // Вкладка при наведенні
    style.Colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.45f, 0.80f, 1.0f); // Активна (вибрана) вкладка
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.30f, 0.50f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    settigs::load();
    actions::LoadAllLists();
    actions::RebuildActionsList();

    // -------------------------------------------------------------
    // НАЛАШТУВАННЯ КОЛЬОРІВ (Вкладки, Лінія, Меню)
    // -------------------------------------------------------------

    // 1. Кольори вкладок (TabBar)
    style.Colors[ImGuiCol_Tab]                = settigs::HexToColor(0x222226); // Неактивна вкладка
    style.Colors[ImGuiCol_TabHovered]         = settigs::HexToColor(0x2E8B57); // Наведення
    style.Colors[ImGuiCol_TabActive]          = settigs::HexToColor(0x2E8B57); // Активна (вибрана) вкладка
    style.Colors[ImGuiCol_TabUnfocused]       = settigs::HexToColor(0x18181A);
    style.Colors[ImGuiCol_TabUnfocusedActive] = settigs::HexToColor(0x1E5637);

    // 2. Синя лінія під вкладками (прибираємо синій колір)
    style.Colors[ImGuiCol_Separator]        = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]  = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);

    // 3. Тло полів вводу
    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);

    // 4. Підсвічування пунктів у списку Combo (прибираємо синій виділений елемент)
    style.Colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.18f, 0.55f, 0.36f, 1.0f); // Зеленавий при наведенні
    style.Colors[ImGuiCol_HeaderActive]   = ImVec4(0.22f, 0.45f, 0.32f, 1.0f); // Зеленавий при кліку

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        settigs::HandleKeybinds();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Зміна базових кольорів рамок (прибирає синій дефолт ImGui)
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_FrameBg]        = settigs::HexToColor(0x252528); // Звичайний стан (темно-сірий замість синього)
        style.Colors[ImGuiCol_FrameBgHovered] = settigs::HexToColor(0x3A3A3E); // При наведенні
        style.Colors[ImGuiCol_FrameBgActive]  = settigs::HexToColor(0x1E5637); // При кліку
        style.Colors[ImGuiCol_CheckMark]      = settigs::HexToColor(0x3CB371); // Галочка (зелена)

        // точка написання гуі
        ImGui::SetNextWindowSize(ImVec2(400 * main_scale, 500 * main_scale));
        ImGui::SetNextWindowPos(ImVec2(0 * main_scale, 0 * main_scale));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::Begin("main main", nullptr, window_flags);

        if (ImGui::BeginTabBar("tabbar")) {

            // ==================== ВКЛАДКА "main" ====================
            if (ImGui::BeginTabItem("main")) {

                // Кнопка "+" (Зелена)
                ImGui::PushStyleColor(ImGuiCol_Button, settigs::HexToColor(0x008000));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, settigs::HexToColor(0x006600));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, settigs::HexToColor(0x004d00));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

                if (ImGui::Button("+", ImVec2(60, 30))) {
                    if (settigs::cout_keybind < 32) {
                        settigs::cout_keybind++;
                        settigs::bind_items.resize(settigs::cout_keybind);
                        settigs::save();
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                // Кнопка "-" (Червона)
                ImGui::SameLine(70.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, settigs::HexToColor(0x8B0000));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, settigs::HexToColor(0xA50000));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, settigs::HexToColor(0xB22222));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

                if (ImGui::Button("-", ImVec2(60, 30))) {
                    if (settigs::cout_keybind > 1) {
                        settigs::cout_keybind--;
                        settigs::save();
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                // Кнопка "update" (Синя)
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, settigs::HexToColor(0x404040));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, settigs::HexToColor(0x333333));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, settigs::HexToColor(0x262626));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

                if (ImGui::Button(u8"update", ImVec2(80, 30))) {
                    actions::ScanZalejnostFolder();
                }

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                ImGui::Text("window cout: %d", settigs::cout_keybind);
                ImGui::Separator();

                // Динамічний вивід біндів
                for (int i = 0; i < settigs::cout_keybind; i++) {
                    ImGui::PushID(i);

                    char button_label[32];
                    if (settigs::listening_index == i) {
                        sprintf_s(button_label, sizeof(button_label), "Натисніть...");
                    } else {
                        std::string key_name = GetKeyName(settigs::bind_items[i].key);
                        sprintf_s(button_label, sizeof(button_label), "[%s]", key_name.c_str());
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, settigs::HexToColor(0x008000));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, settigs::HexToColor(0x006600));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, settigs::HexToColor(0x004d00));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                    if (ImGui::Button(button_label, ImVec2(130, 30))) {
                        settigs::listening_index = i;
                    }

                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();

                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.18f, 0.22f, 1.0f)); // Звичайний стан
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.35f, 1.0f)); // При наведенні
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.35f, 0.45f, 1.0f)); // При натисканні

                    ImGui::SetNextItemWidth(180.0f);
                    if (ImGui::Combo("##action", &settigs::bind_items[i].action, actions::list, actions::count)) {
                        settigs::save();
                    }

                    ImGui::PopStyleColor(3); // Повертаємо стилі назад

                    ImGui::PopID();
                }

                ImGui::EndTabItem(); // Кінець вкладки main
            }

          // ==================== ВКЛАДКА "Settings" ====================
            if (ImGui::BeginTabItem("Settings")) {
                static char exe_buf[260] = "";
                static char http_buf[260] = "";
                static char video_buf[260] = "";
                static char photo_buf[260] = "";


                // ----------------------------------------------------

                settigs::DrawAddableList(u8"EXE файли",         exe_buf,   sizeof(exe_buf),   actions::program_paths, "programs.txt");
                settigs::DrawAddableList(u8"Посилання (HTTP)",  http_buf,  sizeof(http_buf),  actions::link_list,     "links.txt");
                settigs::DrawAddableList(u8"Відео",             video_buf, sizeof(video_buf), actions::video_paths,   "videos.txt");
                settigs::DrawAddableList(u8"Фото",              photo_buf, sizeof(photo_buf), actions::image_paths,   "images.txt");

                ImGui::EndTabItem(); // Кінець вкладки Settings
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}