#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <windows.h> // Для GetAsyncKeyState
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "actions.hpp" // Підключаємо дії
#include <algorithm>






namespace settigs {
    struct KeybindItem {
        int key = 0;
        int action = 0;
    };


    inline ImVec4 HexToColor(unsigned int hex_value, float alpha_value = 1.0f) {
        float r = ((hex_value >> 16) & 0xFF) / 255.0f;
        float g = ((hex_value >> 8) & 0xFF) / 255.0f;
        float b = (hex_value & 0xFF) / 255.0f;
        return ImVec4(r, g, b, alpha_value);
    }
    // Допоміжна функція для отримання тільки назви файлу
    // Оновлене малювання списків у Settings
inline void DrawAddableList(const char* label, char* buffer, size_t buf_size, std::vector<std::string>& vec, const char* filename) {
    if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID(label);

        // Поле вводу + кнопка "Додати"
        ImGui::SetNextItemWidth(230.0f);
        ImGui::InputText("##input", buffer, buf_size);
        ImGui::SameLine();



        // ==================== КОЛІР КНОПКИ "Додати" ====================
        ImGui::PushStyleColor(ImGuiCol_Button,        settigs::HexToColor(0x2E8B57)); // Основний колір (Зелений)
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, settigs::HexToColor(0x3CB371)); // При наведенні
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  settigs::HexToColor(0x1E5637)); // При натисканні

        if (ImGui::Button(u8"Додати")) {
            std::string text = buffer;

            // 1. Видаляємо приховані символи переносу \r та \n від Ctrl+V
            text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
            text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());

            // 2. Обрізаємо пробіли з початку та кінця рядка
            size_t first = text.find_first_not_of(" \t");
            if (first != std::string::npos) {
                size_t last = text.find_last_not_of(" \t");
                text = text.substr(first, (last - first + 1));

                // 3. Додаємо очищений текст посилання
                vec.push_back(text);
                actions::SaveListToFile(filename, vec);
                actions::RebuildActionsList();
                buffer[0] = '\0'; // Очищаємо поле вводу
            }
        }




        ImGui::PopStyleColor(3); // Повертаємо 3 кольори назад!

        // Компактний блок зі скролом (висота 90px)
        if (!vec.empty()) {
            ImGui::BeginChild("##scroll_area", ImVec2(0, 90), true);
            for (size_t i = 0; i < vec.size(); i++) {
                ImGui::PushID((int)i);

                // Червона кнопка видалення [X]
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("X", ImVec2(20, 20))) {
                    vec.erase(vec.begin() + i);
                    actions::SaveListToFile(filename, vec);
                    actions::RebuildActionsList();
                    ImGui::PopStyleColor(2);
                    ImGui::PopID();
                    break; // Перериваємо цикл після видалення елемента
                }
                ImGui::PopStyleColor(2);

                ImGui::SameLine();

                // Перевіряємо, чи це посилання
                bool isLink = (vec[i].rfind("http://", 0) == 0 || vec[i].rfind("https://", 0) == 0);

                // Якщо посилання — виводимо повністю, якщо файл — витягуємо лише його назву
                std::string displayName = isLink ? vec[i] : actions::GetFileName(vec[i]);

                ImGui::TextUnformatted(displayName.c_str());

                // Якщо навести мишкою — покажеться ПОВНИЙ шлях
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", vec[i].c_str());
                }

                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::PopID();
    }
}

    inline int cout_keybind = 1;
    inline std::vector<KeybindItem> bind_items(10);
    inline int listening_index = -1;



    inline bool auto_start = true;

    // Функція для додавання/видалення з автозавантаження Windows
    inline void SetAutoStart(bool enable) {
        HKEY hKey;
        const char* path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (enable) {
                char exePath[MAX_PATH];
                GetModuleFileNameA(NULL, exePath, MAX_PATH);
                std::string cmd = std::string("\"") + exePath + "\" --minimized";
                RegSetValueExA(hKey, "MyHotkeyTool", 0, REG_SZ, (const BYTE*)cmd.c_str(), cmd.size() + 1);
            } else {
                RegDeleteValueA(hKey, "MyHotkeyTool");
            }
            RegCloseKey(hKey);
        }
    }

    inline void save() {
        std::ofstream file(actions::GetExeDir() + "settings.txt");
        if (file.is_open()) {
            file << "AutoStart: " << auto_start << "\n";
            file << "Count_Keybind: " << cout_keybind << "\n";
            for (int i = 0; i < cout_keybind; i++) {
                file << "Key_" << i << ": " << bind_items[i].key << "\n";
                file << "Action_" << i << ": " << bind_items[i].action << "\n";
            }
            file.close();
        }
    }

    inline void load() {
        std::ifstream file(actions::GetExeDir() + "settings.txt");
        if (file.is_open()) {
            std::string dummy;
            file >> dummy >> auto_start;
            file >> dummy >> cout_keybind;
            if (cout_keybind < 1) cout_keybind = 1;
            if (cout_keybind > 32) cout_keybind = 32;

            bind_items.resize(cout_keybind);
            for (int i = 0; i < cout_keybind; i++) {
                file >> dummy >> bind_items[i].key;
                file >> dummy >> bind_items[i].action;
            }
            file.close();
        }
    }
    inline std::string GetExeDir() {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string path(exePath);
        size_t pos = path.find_last_of("\\/");
        return (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
    }


    // Повністю розширена конвертація GLFW кодів у системні Windows Virtual Keys (VK_*)
    inline int GlfwKeyToVK(int glfwKey) {
        // Стандартні літери та цифри
        if (glfwKey >= 'A' && glfwKey <= 'Z') return glfwKey;
        if (glfwKey >= '0' && glfwKey <= '9') return glfwKey;

        // Функціональні клавіші F1 - F12
        if (glfwKey >= GLFW_KEY_F1 && glfwKey <= GLFW_KEY_F12) {
            return VK_F1 + (glfwKey - GLFW_KEY_F1);
        }

        // Стрілки
        if (glfwKey == GLFW_KEY_UP) return VK_UP;
        if (glfwKey == GLFW_KEY_DOWN) return VK_DOWN;
        if (glfwKey == GLFW_KEY_LEFT) return VK_LEFT;
        if (glfwKey == GLFW_KEY_RIGHT) return VK_RIGHT;

        // Основні системні клавіші
        if (glfwKey == GLFW_KEY_SPACE) return VK_SPACE;
        if (glfwKey == GLFW_KEY_ESCAPE) return VK_ESCAPE;
        if (glfwKey == GLFW_KEY_ENTER) return VK_RETURN;
        if (glfwKey == GLFW_KEY_TAB) return VK_TAB;
        if (glfwKey == GLFW_KEY_BACKSPACE) return VK_BACK;
        if (glfwKey == GLFW_KEY_LEFT_SHIFT || glfwKey == GLFW_KEY_RIGHT_SHIFT) return VK_SHIFT;
        if (glfwKey == GLFW_KEY_LEFT_CONTROL ||glfwKey == GLFW_KEY_RIGHT_CONTROL) return VK_CONTROL;
        if (glfwKey == GLFW_KEY_LEFT_ALT || glfwKey == GLFW_KEY_RIGHT_ALT) return VK_MENU;

        // Навігація та редагування
        if (glfwKey == GLFW_KEY_INSERT) return VK_INSERT;
        if (glfwKey == GLFW_KEY_DELETE) return VK_DELETE;
        if (glfwKey == GLFW_KEY_HOME) return VK_HOME;
        if (glfwKey == GLFW_KEY_END) return VK_END;
        if (glfwKey == GLFW_KEY_PAGE_UP) return VK_PRIOR;
        if (glfwKey == GLFW_KEY_PAGE_DOWN) return VK_NEXT;

        // Додатковий блок Numpad
        if (glfwKey >= GLFW_KEY_KP_0 && glfwKey <= GLFW_KEY_KP_9) return VK_NUMPAD0 + (glfwKey - GLFW_KEY_KP_0);
        if (glfwKey == GLFW_KEY_KP_DECIMAL) return VK_DECIMAL;
        if (glfwKey == GLFW_KEY_KP_DIVIDE) return VK_DIVIDE;
        if (glfwKey == GLFW_KEY_KP_MULTIPLY) return VK_MULTIPLY;
        if (glfwKey == GLFW_KEY_KP_SUBTRACT) return VK_SUBTRACT;
        if (glfwKey == GLFW_KEY_KP_ADD) return VK_ADD;
        if (glfwKey == GLFW_KEY_KP_ENTER) return VK_RETURN;

        // Символи та знаки пунктуації
        if (glfwKey == GLFW_KEY_MINUS) return VK_OEM_MINUS;
        if (glfwKey == GLFW_KEY_EQUAL) return VK_OEM_PLUS;
        if (glfwKey == GLFW_KEY_LEFT_BRACKET) return VK_OEM_4;
        if (glfwKey == GLFW_KEY_RIGHT_BRACKET) return VK_OEM_6;
        if (glfwKey == GLFW_KEY_SEMICOLON) return VK_OEM_1;
        if (glfwKey == GLFW_KEY_APOSTROPHE) return VK_OEM_7;
        if (glfwKey == GLFW_KEY_GRAVE_ACCENT) return VK_OEM_3;
        if (glfwKey == GLFW_KEY_BACKSLASH) return VK_OEM_5;
        if (glfwKey == GLFW_KEY_COMMA) return VK_OEM_COMMA;
        if (glfwKey == GLFW_KEY_PERIOD) return VK_OEM_PERIOD;
        if (glfwKey == GLFW_KEY_SLASH) return VK_OEM_2;

        return 0;
    }

    inline void HandleKeybinds() {
        if (listening_index != -1) return;

        static bool wasPressed[32] = { false };

        for (int i = 0; i < cout_keybind; i++) {
            int key = bind_items[i].key;
            int action = bind_items[i].action;

            if (key != 0 && action != 0) {
                int vk_key = GlfwKeyToVK(key);
                if (vk_key != 0) {
                    bool isPressedNow = (GetAsyncKeyState(vk_key) & 0x8000) != 0;

                    if (isPressedNow && !wasPressed[i]) {
                        actions::Execute(action); // Виконуємо дію рівно 1 раз за натискання
                    }

                    wasPressed[i] = isPressedNow;
                }
            } else {
                wasPressed[i] = false;
            }
        }
    }
}