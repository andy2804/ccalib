#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "include/camera.h"
#include <stdio.h>
#include <vector>
#include <iostream>
#include <SDL.h>
#include <experimental/filesystem>
#include <glad/glad.h>  // Initialize with gladLoadGL()

using namespace std;
namespace fs = std::experimental::filesystem;

void mat2Texture(cv::Mat &image, GLuint &imageTexture) {
    if (image.empty()) {
        std::cout << "image empty" << std::endl;
    } else {
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Set texture clamping method
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

//        cv::cvtColor(image, image, CV_RGB2BGR);

        glTexImage2D(GL_TEXTURE_2D,         // Type of texture
                     0,                   // Pyramid level (for mip-mapping) - 0 is the top level
                     GL_RGB,              // Internal colour format to convert to
                     image.cols,          // Image width  i.e. 640 for Kinect in standard mode
                     image.rows,          // Image height i.e. 480 for Kinect in standard mode
                     0,                   // Border width in pixels (can either be 1 or 0)
                     GL_RGB,              // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                     GL_UNSIGNED_BYTE,    // Image data type
                     image.ptr());        // The actual image data itself
    }
}

// Main code
int main(int, char **) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("ccalib", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                          window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif

    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext *ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    // Set custom style elements
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowTitleAlign = ImVec2(0.5, 0.5);
    style.WindowRounding = 0;
    style.FrameRounding = 5;
    style.GrabRounding = 5;
    style.PopupRounding = 5;
    style.ScrollbarRounding = 8;
    style.WindowBorderSize = 1;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(8, 6);
    style.FrameBorderSize = 1;
    style.ItemSpacing = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(8, 8);
    style.GrabMinSize = 5;
    style.ScrollbarSize = 12;

    // Set custom colors
    ImVec4 *colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.93f, 0.93f, 0.93f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.16f, 0.03f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.53f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.87f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.75f, 0.75f, 0.75f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.20f, 0.20f, 0.86f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.70f, 0.00f, 0.50f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 0.70f, 0.00f, 0.20f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.70f, 0.00f, 0.50f);
    colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.71f, 0.71f, 0.71f, 0.91f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.70f, 0.00f, 0.50f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.70f, 0.00f, 0.50f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.20f, 0.20f, 0.39f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    io.Fonts->AddFontFromFileTTF("../resources/Roboto-Regular.ttf", 16.0f);

    // State variables
    bool show_demo_window = false;
    bool calibration_mode = false;
    bool stream_on = false;
    bool changed = false;
    int camera_curr = 0;
    int camera_width = 640;
    int camera_height = 480;
    int camera_fps = 30;
    int camera_exposure = 33;   // in [ms]
    bool camera_autoexp = false;
    int camera_currfmt = 0;
    int chkbrd_rows = 8;
    int chkbrd_cols = 11;
    float img_ratio = (float) camera_width / (float) camera_height;
    int width_parameter_window = 400;
    vector<string> camera_fmt{"YUVY", "YUY2", "Y16 ", "MJPG", "MPEG", "X264", "HEVC"};
    vector<cv::Point2f> corners;
    cv::Mat img = cv::Mat::zeros(cv::Size(camera_width, camera_height), CV_8UC3);
    GLuint texture;

    // Get all v4l2 devices
    const fs::path device_dir("/dev");
    vector<string> cameras;

    for (const auto &entry : fs::directory_iterator(device_dir)) {
        if (entry.path().string().find("video") != string::npos)
            cameras.push_back(entry.path());
    }

    // Try to open camera connection2
    cv::VideoCapture camera(camera_curr);
    if (!camera.isOpened())
        CV_Assert("Cam open failed");

    camera.set(CV_CAP_PROP_FRAME_WIDTH, camera_width);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, camera_height);
    camera.set(CV_CAP_PROP_FPS, camera_fps);
//    camera.set(CV_CAP_PROP_EXPOSURE, 0.25);
    camera.set(CV_CAP_PROP_AUTO_EXPOSURE, camera_autoexp);
//    V4L2Camera camera = V4L2Camera(cameras[0], 640, 480);
//    camera.setFramerate(framerate);
//    camera.allocateBuffer();
//    camera.startStream();

    // Main loop
//    SDL_SetWindowSize(window, width_parameter_window + img.cols, max(720, img.rows));
    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Show parameters window
        {
            // Set next window size & pos
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(width_parameter_window, io.DisplaySize.y), ImGuiCond_Always);

            ImGui::Begin("Parameters", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar);

            // Camera selector
            ImGui::AlignTextToFramePadding();
            ImGui::PushItemWidth(-1);
            ImGui::Text("Select Device");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##camera_selector", cameras[camera_curr].c_str(), 0)) {
                for (int i = 0; i < cameras.size(); i++) {
                    bool is_selected = (cameras[camera_curr] == cameras[i]);
                    if (ImGui::Selectable(cameras[i].c_str(), is_selected))
                        camera_curr = i;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
            }

            // Camera Controls
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Camera Control");
            ImGui::SameLine();
            if (ImGui::Button("Start / Stop Stream")) {
                stream_on = !stream_on;
                if (stream_on) {
                    camera.open(camera_curr);
                    camera.set(CV_CAP_PROP_FRAME_WIDTH, (double) camera_width);
                    camera.set(CV_CAP_PROP_FRAME_HEIGHT, (double) camera_height);
                    camera.grab();
                    camera.retrieve(img);

                    // Update params
                    camera_width = img.cols;
                    camera_height = img.rows;
                    img_ratio = (float) camera_width / (float) camera_height;
//                    SDL_SetWindowSize(window, width_parameter_window + img.cols, max(720, img.rows));
                } else
                    camera.release();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Width");
            ImGui::SameLine();
            ImGui::PushItemWidth(64);
            if (ImGui::InputInt("##camera_width", &camera_width, 0))
                changed = true;
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Height");
            ImGui::SameLine();
            if (ImGui::InputInt("##camera_height", &camera_height, 0))
                changed = true;
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Exposure Time");
            ImGui::SameLine();
            if (ImGui::SliderInt("##camera_exptime", &camera_exposure, 0, 1000, "%d [ms]")) {
                changed = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Select Format");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##camera_fmt", camera_fmt[camera_currfmt].c_str(), 0)) {
                for (int i = 0; i < camera_fmt.size(); i++) {
                    bool is_selected = (camera_fmt[camera_currfmt] == camera_fmt[i]);
                    if (ImGui::Selectable(camera_fmt[i].c_str(), is_selected))
                        camera_currfmt = i;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
                changed = true;
            }

            if (changed) {
                stream_on = false;
                calibration_mode = false;
//                int32_t fourcc = 0;
//                for (const char& c : camera_fmt[camera_currfmt]) {
//                    fourcc <<= 8;
//                    fourcc += c;
//                }
//                camera.set(CV_CAP_PROP_FOURCC, fourcc);
//                camera.set(CV_CAP_PROP_EXPOSURE, camera_exposure);
                changed = false;
            }

            if (stream_on) {
                camera.grab();
                camera.retrieve(img);

                cv::cvtColor(img, img, CV_BGR2RGB);
            }

            // Camera Controls
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Calibration");
            ImGui::SameLine();
            if (ImGui::Button("Start / Stop")) {
                calibration_mode = !calibration_mode;
            }

            if (calibration_mode && stream_on) {
                cv::Mat gray(img.rows, img.cols, CV_8UC1);
                cv::cvtColor(img, gray, cv::COLOR_RGB2GRAY);
                if (cv::findChessboardCorners(gray, cv::Size(chkbrd_cols - 1, chkbrd_rows - 1), corners,
                                              CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE |
                                              CV_CALIB_CB_FAST_CHECK)) {
                    cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                                     cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
                    cv::drawChessboardCorners(img, cv::Size(chkbrd_cols - 1, chkbrd_rows - 1), cv::Mat(corners), true);
                }
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Rows");
            ImGui::SameLine();
            ImGui::PushItemWidth(64);
            ImGui::InputInt("##chkbrd_rows", &chkbrd_rows, 0);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Cols");
            ImGui::SameLine();
            ImGui::InputInt("##chkbrd_cols", &chkbrd_cols, 0);
            ImGui::PopItemWidth();


            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFontSize() - 8);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Show image preview
        {
            // TODO Only render if active camera connection

            // Set next window size & pos
//            ctx->CurrentWindow->Size.x = width_parameter_window + img.cols;
            ImGui::SetNextWindowPos(ImVec2(width_parameter_window, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - width_parameter_window, io.DisplaySize.y),
                                     ImGuiCond_Always);

            ImGui::Begin("Preview", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar);

            // Camera image
            if (stream_on) {
                glDeleteTextures(1, &texture);
                mat2Texture(img, texture);
            }
            if (ImGui::GetWindowHeight() * img_ratio > ImGui::GetWindowWidth())
                cv::resize(img, img,
                           cv::Size((int) ImGui::GetWindowWidth(), (int) (ImGui::GetWindowWidth() / img_ratio)));
            else
                cv::resize(img, img,
                           cv::Size((int) (ImGui::GetWindowHeight() * img_ratio), (int) ImGui::GetWindowHeight()));

//            img = camera.captureRawFrame();
//            cv::resize(img, img, cv::Size((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

            ImGui::SetCursorPos(
                    ImVec2((ImGui::GetWindowWidth() - img.cols) / 2, (ImGui::GetWindowHeight() - img.rows) / 2));
            ImGui::Image((void *) (intptr_t) texture, ImVec2(img.cols, img.rows));

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
//        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
