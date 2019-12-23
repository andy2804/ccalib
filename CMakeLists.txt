cmake_minimum_required(VERSION 3.10)

project(ccalib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs")

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED)

add_library("glad" "glad/src/glad.c")

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    "glad/include"
)

set(sources
    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_widgets.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_opengl3.h
    imgui/imgui_impl_sdl.cpp
    imgui/imgui_impl_sdl.h
        camera.cpp
        include/camera.h
    )

add_executable(ccalib main.cpp ${sources})

target_link_libraries(${CMAKE_PROJECT_NAME}
        "glad"
        ${SDL2_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${CMAKE_DL_LIBS}
        ${OpenCV_LIBRARIES}
        stdc++fs
        )

