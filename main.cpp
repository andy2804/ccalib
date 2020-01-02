#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "include/camera.h"
#include <ctime>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <SDL.h>
#include <experimental/filesystem>
#include <glad/glad.h>  // Initialize with gladLoadGL()
#include <numeric>

using namespace std;
namespace fs = std::experimental::filesystem;

struct snapshot {
    timeval id;
    cv::Mat img;
    vector<cv::Point2f> corners;
};

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

constexpr uint32_t fourcc(char const p[5]) {
    return (((p[0]) & 255) + (((p[1]) & 255) << 8) + (((p[2]) & 255) << 16) + (((p[3]) & 255) << 24));
}

bool findCorners(cv::Mat &img, vector<cv::Point2f> &corners, const int &cols, const int &rows) {
    if (cv::findChessboardCorners(img, cv::Size(cols - 1, rows - 1), corners,
                                  CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE |
                                  CV_CALIB_CB_FAST_CHECK)) {
        cv::cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
                         cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
        return true;
    }
    return false;
}

void ToggleButton(const char *str_id, bool *v) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight() * 0.8f;
    float width = height * 1.8f;
    float radius = height * 0.50f;
    p.y += 0.1f * ImGui::GetFrameHeight();

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;

    float t = *v ? 1.0f : 0.0f;

    ImGuiContext &g = *GImGui;
    float ANIM_SPEED = 0.08f;
    if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    {
        float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
        t = *v ? (t_anim) : (1.0f - t_anim);
    }

    ImU32 col_bg;
    if (ImGui::IsItemHovered())
        col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
    else
        col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f,
                               IM_COL32(255, 255, 255, 255));
}

void CoveredBar(const float &start, const float &stop, const float &indicator = -1) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight() * 0.25f;
    float width = ImGui::GetContentRegionAvailWidth();
    p.y += 0.375f * ImGui::GetFrameHeight();

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height),
                             ImGui::GetColorU32(ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
    if (stop > start) {
        draw_list->AddRectFilled(ImVec2(p.x + max(0.0f, start) * width, p.y),
                                 ImVec2(p.x + min(1.0f, stop) * width, p.y + height),
                                 ImGui::GetColorU32(ImVec4(0.56f, 0.83f, 0.26f, 1.0f)), height * 0.5f);
    }

    if (indicator >= 0)
        draw_list->AddCircleFilled(ImVec2(p.x + indicator * width, p.y + height / 2.0f), height,
                                   ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

    ImGui::InvisibleButton("##covered_bar", ImVec2(width, ImGui::GetFrameHeight()));
}

bool MaterialButton(const char *label, bool focus = false, const ImVec2 &size = ImVec2(0, 0)) {
    // Get Position and Drawlist
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();


    // Draw button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.56f, 0.83f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.75f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    bool clicked = ImGui::Button(label, size);
    float height = ImGui::GetItemRectSize().y;
    float width = ImGui::GetItemRectSize().x;

    ImGui::PopStyleColor(3);

    // Draw rectangle if focused
    if (focus) {
        ImU32 col_bg;
        ImGuiContext &g = *GImGui;
        float padding = 1.0f;
        float ANIM_SPEED = 0.32f;
        float t_anim = cos(g.LastActiveIdTimer / ANIM_SPEED);
        col_bg = ImGui::GetColorU32(
                ImLerp(ImVec4(0.56f, 0.83f, 0.26f, 1.0f), ImVec4(0.72f, 0.91f, 0.42f, 1.0f), t_anim));

        draw_list->AddRect(ImVec2(p.x - padding, p.y - padding),
                           ImVec2(p.x + width + padding, p.y + height + padding), col_bg,
                           ImGui::GetStyle().FrameRounding,
                           ImDrawCornerFlags_All, padding * 2);
    }

    return clicked;
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
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
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
    colors[ImGuiCol_CheckMark] = ImVec4(0.64f, 0.83f, 0.34f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.20f, 0.20f, 0.86f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.15f, 0.64f, 1.00f, 0.80f);
    colors[ImGuiCol_Button] = ImVec4(0.81f, 0.81f, 0.81f, 0.49f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.56f, 0.83f, 0.26f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.64f, 0.83f, 0.34f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.81f, 0.81f, 0.81f, 0.49f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.83f, 0.26f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.72f, 0.83f, 0.42f, 1.0f);
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
    bool camera_on = false;
    bool stream_on = false;
    bool changed = false;
    bool flip_img = false;
    bool calibrated = false;
    bool taking_snapshot = false;

    int camera_curr = 0;
    int camera_width = 640;
    int camera_height = 480;
    float camera_exposure = 0.333;   // in [ms]
    bool camera_autoexp = false;
    int camera_actfps = 30;
    int camera_currfps = 4;
    int camera_currfmt = 0;

    int chkbrd_rows = 8;
    int chkbrd_cols = 11;
    float chkbrd_size = 0.022;      // in [m]

    float x_min = camera_width;
    float x_max = 0;
    float y_min = camera_height;
    float y_max = 0;
    float size_min = camera_width * camera_height;
    float size_max = 0;
    float max_size = size_min;

    // Initialize camera matrix && dist coeff
    cv::Mat K = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat D = cv::Mat::zeros(8, 1, CV_64F);
    vector<cv::Mat> R, T;
    double rms;

    int snapshot_curr = -1;
    vector<snapshot> instances;

    cv::Point2f mean(camera_width / 2, camera_height / 2);
    float size = max_size / 2;

    float img_ratio = (float) camera_width / (float) camera_height;
    int width_parameter_window = 320;
    vector<int> camera_fps{5, 10, 15, 20, 30, 50, 60, 100, 120};
    vector<string> camera_fmt{"YUVY", "YUY2", "YU12", "YV12", "RGB3", "BGR3", "Y16 ", "MJPG", "MPEG", "X264", "HEVC"};
    vector<cv::Point2f> corners;
    cv::Mat img = cv::Mat::zeros(cv::Size(camera_width, camera_height), CV_8UC3);
    cv::Mat img_prev = cv::Mat::zeros(cv::Size(camera_width, camera_height), CV_8UC3);
    GLuint texture;

    // Get all v4l2 devices
    const fs::path device_dir("/dev");
    vector<string> cameras;

    for (const auto &entry : fs::directory_iterator(device_dir)) {
        if (entry.path().string().find("video") != string::npos)
            cameras.push_back(entry.path());
    }

    // Try to open camera connection
    cv::VideoCapture camera(camera_curr);
    if (!camera.isOpened())
        CV_Assert("Cam open failed");


    // Main loop
    bool done = false;
    while (!done) {
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
            ImGui::Text("Camera");
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8);
            ToggleButton("##cam_toggle", &camera_on);
            if (camera_on && ImGui::IsItemClicked(0)) {
                camera.open(camera_curr);
                camera.set(CV_CAP_PROP_FOURCC, fourcc(camera_fmt[camera_currfmt].c_str()));
                camera.set(CV_CAP_PROP_FRAME_WIDTH, (double) camera_width);
                camera.set(CV_CAP_PROP_FRAME_HEIGHT, (double) camera_height);

                camera.set(CV_CAP_PROP_AUTO_EXPOSURE, 0.25);
                camera.set(CV_CAP_PROP_EXPOSURE, camera_exposure);
                camera.grab();
                camera.retrieve(img);

                stream_on = true;
                changed = true;
            } else if (!camera_on) {
                camera.release();
                stream_on = false;
            }

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Resolution");
            ImGui::SameLine();
            ImGui::PushItemWidth(48);
            ImGui::InputInt("##width", &camera_width, 0);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("x");
            ImGui::SameLine();
            ImGui::InputInt("##height", &camera_height, 0);
            ImGui::SameLine(ImGui::GetWindowWidth() - 44);
            if (MaterialButton("Set")) {
                camera.set(CV_CAP_PROP_FRAME_WIDTH, (double) camera_width);
                camera.set(CV_CAP_PROP_FRAME_HEIGHT, (double) camera_height);
                changed = true;
            }
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Framerate");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##camera_fps", to_string(camera_fps[camera_currfps]).c_str(), 0)) {
                for (int i = 0; i < camera_fps.size(); i++) {
                    bool is_selected = (camera_fps[camera_currfps] == camera_fps[i]);
                    if (ImGui::Selectable(to_string(camera_fps[i]).c_str(), is_selected)) {
                        camera_currfps = i;
                        camera.set(CV_CAP_PROP_FPS, (double) camera_fps[camera_currfps]);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
                changed = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Exposure Time");
            ImGui::SameLine();
            if (ImGui::SliderFloat("##camera_exptime", &camera_exposure, 0, 1, "%.3f", 2.0)) {
                camera.set(CV_CAP_PROP_EXPOSURE, camera_exposure / 1.25);
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Select Format");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##camera_fmt", camera_fmt[camera_currfmt].c_str(), 0)) {
                for (int i = 0; i < camera_fmt.size(); i++) {
                    bool is_selected = (camera_fmt[camera_currfmt] == camera_fmt[i]);
                    if (ImGui::Selectable(camera_fmt[i].c_str(), is_selected)) {
                        camera_currfmt = i;
                        camera.set(CV_CAP_PROP_FOURCC, fourcc(camera_fmt[camera_currfmt].c_str()));
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
                changed = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Flip Image");
            ImGui::SameLine();
            ImGui::Checkbox("##flip", &flip_img);

            if (changed && camera_on) {
                // Update params
                camera.grab();
                camera.retrieve(img);
                camera_width = img.cols;
                camera_height = img.rows;
                img_ratio = (float) camera_width / (float) camera_height;
                camera_actfps = camera.get(CV_CAP_PROP_FPS);
                for (int i = 0; i < camera_fps.size(); i++)
                    camera_currfps = (camera_fps[i] == camera_actfps) ? i : camera_currfps;
                changed = false;
            }

            if (stream_on) {
                camera.grab();
                camera.retrieve(img);
                cv::cvtColor(img, img, CV_BGR2RGB);
            }

            ImGui::Separator();

            // Calibration Controls
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Calibration");
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8);
            ToggleButton("##calib_toggle", &calibration_mode);
            if (camera_on && calibration_mode && ImGui::IsItemClicked(0)) {
                x_min = camera_width;
                x_max = 0;
                y_min = camera_height;
                y_max = 0;
                size_min = camera_width * camera_height;
                size_max = 0;
                max_size = sqrt(size_min * 0.9f);
            } else if (!camera_on)
                calibration_mode = false;

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Rows");
            ImGui::SameLine();
//            ImGui::PushItemWidth(48);
            ImGui::InputInt("##chkbrd_rows", &chkbrd_rows, 1);
//            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Cols");
            ImGui::SameLine();
            ImGui::InputInt("##chkbrd_cols", &chkbrd_cols, 1);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Size in [m]");
            ImGui::SameLine();
            ImGui::InputFloat("##chkbrd_size", &chkbrd_size, 0.001f);
//            ImGui::PopItemWidth();

            if (calibration_mode) {
                if (stream_on) {
                    cv::Mat gray(img.rows, img.cols, CV_8UC1);
                    cv::cvtColor(img, gray, cv::COLOR_RGB2GRAY);
                    cv::cvtColor(gray, img, cv::COLOR_GRAY2RGB);
//                    cv::resize(gray, gray, cv::Size(gray.cols / 2, gray.rows / 2));
                    if (findCorners(gray, corners, chkbrd_cols, chkbrd_rows))
                        cv::drawChessboardCorners(img, cv::Size(chkbrd_cols - 1, chkbrd_rows - 1), cv::Mat(corners),
                                                  true);

                    if (corners.size() > 0) {
                        cv::RotatedRect rect = cv::minAreaRect(corners);
                        mean = rect.center;
                        size = sqrt(rect.size.area());
                    }
                }

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Horizontal Coverage");

                CoveredBar((x_min - (0.1f * camera_width)) / camera_width,
                           (x_max + (0.1f * camera_width)) / camera_width,
                           (camera_width - mean.x) / camera_width);

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Vertical Coverage");

                CoveredBar((y_min - (0.1f * camera_height)) / camera_height,
                           (y_max + (0.1f * camera_height)) / camera_height,
                           (camera_height - mean.y) / camera_height);

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Size Coverage");
                CoveredBar((size_min - (0.1f * max_size)) / max_size,
                           (size_max + (0.1f * max_size)) / max_size,
                           (size / max_size));

                ImGui::Separator();

                // Collect snapshot button
                if (MaterialButton("Snapshot", !calibrated && instances.size() < 4) &&
                    corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1)) && stream_on)
                    taking_snapshot = true;

                if (taking_snapshot && stream_on && corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1))) {
                    ImGui::SameLine();
                    ImGui::Text("Try not to move...");

                    // Compare actual frame with previous frame for movement
                    cv::Rect rect = cv::minAreaRect(corners).boundingRect();
                    cv::Mat old_img;
                    cv::Mat new_img;
                    if (rect.x > 0 && rect.y > 0 && rect.x + rect.width < camera_width && rect.y + rect.height < camera_height) {
                        old_img = img_prev(rect);
                        new_img = img(rect);
                    } else {
                        img_prev.copyTo(old_img);
                        img.copyTo(new_img);
                    }
                    cv::cvtColor(old_img, old_img, cv::COLOR_RGB2GRAY);
                    cv::cvtColor(new_img, new_img, cv::COLOR_RGB2GRAY);
                    cv::Mat diff(old_img.rows, old_img.cols, CV_8UC1);
                    cv::absdiff(old_img, new_img, diff);
                    cv::Scalar mean_diff = cv::mean(diff);
                    CoveredBar(0.0f, 1.0f - (float) mean_diff.val[0] / 127.0f);

                    // If successful, add instance
                    if (mean_diff.val[0] < 3) {
                        snapshot instance;
                        gettimeofday(&instance.id, NULL);
                        img.copyTo(instance.img);
                        if (flip_img)
                            cv::flip(instance.img, instance.img, 1);
                        instance.corners.assign(corners.begin(), corners.end());
                        instances.push_back(instance);

                        x_min = min(x_min, camera_width - mean.x);
                        x_max = max(x_max, camera_width - mean.x);
                        y_min = min(y_min, camera_height - mean.y);
                        y_max = max(y_max, camera_height - mean.y);
                        size_min = min(size_min, size);
                        size_max = max(size_max, size);

                        taking_snapshot = false;
                    }
                } else {
                    ImGui::SameLine();
                    if (!corners.empty())
                        ImGui::Text("Ready");
                    else
                        ImGui::Text("No Checkerboard detected!");
                    CoveredBar(0.0f, 0.0f);
                }

                // List all snapshots
                if (instances.size() > 0) {
                    ImGui::ListBoxHeader("Snapshots", instances.size(), (int) min(5.0f, (float) instances.size()));
                    for (int i = 0; i < instances.size(); i++) {
                        bool is_selected = (i == snapshot_curr) ? true : false;
                        double stamp = instances[i].id.tv_sec + (instances[i].id.tv_usec / 1e6);
                        if (ImGui::Selectable(to_string(stamp).c_str(), is_selected)) {
                            if (is_selected)
                                snapshot_curr = -1;
                            else
                                snapshot_curr = i;
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::Selectable("Remove")) {
                                int id = i;
                                instances.erase(instances.begin() + id);
                                snapshot_curr--;
                            }
                            ImGui::EndPopup();
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::ListBoxFooter();
                }

                // Calibrate Using snapshots if enough (min is 4 to solve for 8 DOF)
                if (instances.size() >= 4) {
                    ImGui::Separator();
                    if (MaterialButton("Calibrate", !calibrated)) {
                        // Initialize values
                        vector<cv::Point3f> corners3d;
                        for (int i = 0; i < chkbrd_rows - 1; ++i)
                            for (int j = 0; j < chkbrd_cols - 1; ++j)
                                corners3d.push_back(cv::Point3f(float(j * chkbrd_size), float(i * chkbrd_size), 0));

                        vector<vector<cv::Point2f>> imgPoints;
                        vector<vector<cv::Point3f>> objPoints;
                        for (const auto &instance : instances) {
                            imgPoints.push_back(instance.corners);
                            objPoints.push_back(corners3d);
                        }

                        rms = cv::calibrateCamera(objPoints, imgPoints, cv::Size(camera_width, camera_height),
                                                  K, D, R, T, CV_CALIB_FIX_ASPECT_RATIO |
                                                              CV_CALIB_FIX_K4 |
                                                              CV_CALIB_FIX_K5);

                        if (!calibrated)
                            calibrated = true;
                    }

                    if (calibrated) {
                        if (MaterialButton("Export", calibrated)) {
                            cv::FileStorage fs("calibration.yaml", cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);
                            fs << "image_width" << camera_width;
                            fs << "image_height" << camera_height;
                            fs << "camera_name" << cameras[camera_curr];
                            fs << "camera_matrix" << K;
                            fs << "distortion_model" << "plumb_bob";
                            fs << "distortion_coefficients" << D;
                            fs << "rectification_matrix" << cv::Mat::eye(3, 3, CV_64F);
                        }
                        stringstream result_ss;
                        result_ss << "K = " << K << endl << endl;
                        result_ss << "D = " << D << endl << endl;
                        result_ss << "RMS = " << rms;
                        string result = result_ss.str();
                        char output[result.size() + 1];
                        strcpy(output, result.c_str());
                        ImGui::InputTextMultiline("##result", output, result.size(),
                                                  ImVec2(0, ImGui::GetTextLineHeight() * 11),
                                                  ImGuiInputTextFlags_ReadOnly);
                    }
                }

                if (snapshot_curr != -1) {
                    stream_on = false;
                    img = instances[snapshot_curr].img;
                } else if (camera.isOpened() && !stream_on) {
                    stream_on = true;
                    camera.grab();
                    camera.retrieve(img);
                    cv::cvtColor(img, img, CV_BGR2RGB);
                }
            }

            img.copyTo(img_prev);

            ImGui::NewLine();

//            ImGui::SetCursorPosY(
//                    max(0.0f, (ImGui::GetContentRegionMax().y - min(0.0f, ImGui::GetContentRegionAvail().y) -
//                               ImGui::GetFontSize() + ImGui::GetScrollY())));
//            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
//                        ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Show image preview
        {
            // TODO Only render if active camera connection

            // Set next window size & pos
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::SetNextWindowPos(ImVec2(width_parameter_window, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - width_parameter_window, io.DisplaySize.y),
                                     ImGuiCond_Always);

            ImGui::Begin("Preview", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar);

            if (camera_on) {
                if (flip_img && stream_on)
                    cv::flip(img, img, 1);
                glDeleteTextures(1, &texture);
                mat2Texture(img, texture);
            }

            // Camera image
            if (!img.empty()) {
                if (ImGui::GetWindowHeight() * img_ratio > ImGui::GetWindowWidth())
                    cv::resize(img, img,
                               cv::Size((int) ImGui::GetWindowWidth(), (int) (ImGui::GetWindowWidth() / img_ratio)));
                else
                    cv::resize(img, img,
                               cv::Size((int) (ImGui::GetWindowHeight() * img_ratio), (int) ImGui::GetWindowHeight()));
            }

            ImGui::SetCursorPos(
                    ImVec2((ImGui::GetWindowWidth() - img.cols) / 2, (ImGui::GetWindowHeight() - img.rows) / 2));
            ImGui::Image((void *) (intptr_t) texture, ImVec2(img.cols, img.rows));

            ImGui::End();
            ImGui::PopStyleColor(1);
        }

        // Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

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
