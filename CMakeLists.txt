cmake_minimum_required(VERSION 3.10)

project(ccalib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs")

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED)

add_library("glad" "include/glad/src/glad.c")

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    "include/glad/include"
    "include"
)

set(sources
    include/imgui/imconfig.h
    include/imgui/imgui.cpp
    include/imgui/imgui.h
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_internal.h
    include/imgui/imgui_widgets.cpp
    include/imgui/imstb_rectpack.h
    include/imgui/imstb_textedit.h
    include/imgui/imstb_truetype.h
    include/imgui/imgui_impl_opengl3.cpp
    include/imgui/imgui_impl_opengl3.h
    include/imgui/imgui_impl_sdl.cpp
    include/imgui/imgui_impl_sdl.h
        src/camera.cpp
        src/camera.h
    )

add_executable(ccalib src/main.cpp ${sources})

target_link_libraries(${CMAKE_PROJECT_NAME}
        "glad"
        ${SDL2_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${CMAKE_DL_LIBS}
        ${OpenCV_LIBRARIES}
        stdc++fs
        )

