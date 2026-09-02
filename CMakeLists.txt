cmake_minimum_required(VERSION 3.15)

# Назва твого проєкту та мови програмування
project(hotkey_tool_v2 VERSION 1.0 LANGUAGES C CXX)

# Встановлюємо стандарт C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматично завантажуємо GLFW
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.8
)
# Вимикаємо зайві збірки тестів/документації
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Створюємо наш виконуваний файл exe
add_executable(hotkey_tool_v2
        main.cpp


        # Файли самої бібліотеки
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp

        # Бекенд
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl2.cpp
)

# 6. Вказуємо компілятору, де шукати заголовочні файли hpp h
target_include_directories(hotkey_tool_v2 PRIVATE
        imgui
        imgui/backends

)

# 7. Підключаємо (лінкуємо) бібліотеки до нашого exe
# Для Windows обов'язково потрібен opengl32 та glfw
if(WIN32)
    target_link_libraries(hotkey_tool_v2 PRIVATE glfw opengl32 winmm shell32 gdiplus)
    target_link_options(hotkey_tool_v2 PRIVATE -mwindows)
endif()