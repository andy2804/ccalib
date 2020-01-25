//
// Created by andy2804 on 19.12.19.
//

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "camera.h"
#include "structures.h"
#include "functions.h"
#include "imgui_extensions.h"
#include "calibrator.h"

#include <ctime>
#include <cstdio>
#include <vector>
#include <iostream>
#include <SDL.h>
#include <experimental/filesystem>
#include <glad/glad.h>  // Initialize with gladLoadGL()
#include <numeric>
#include <stack>

using namespace std;
namespace fs = std::experimental::filesystem;

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
    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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
    ImGuiStyle style = ImGui::GetStyle();
    (void) io;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    ccalib::StyleColorsMaterial();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    ImFont *font_normal = io.Fonts->AddFontFromFileTTF("../resources/Roboto-Regular.ttf", 16.0f);
    ImFont *font_title = io.Fonts->AddFontFromFileTTF("../resources/Roboto-Medium.ttf", 24.0f);

    // general State variables
    bool show_demo_window = false;
    bool show_camera_card = true;
    bool show_parameters_card = false;
    bool show_calibration_card = true;
    bool show_coverage_card = false;
    bool show_snapshots_card = true;
    bool show_result_card = true;
    bool calibration_mode = false;
    bool changed = false;
    bool cameraOn = false;
    bool flip_img = false;
    bool undistort = false;
    bool calibrated = false;
    bool taking_snapshot = false;
    bool in_target = false;

    // camera specific state variables
    int camID = 0;
    int camFPS = 4;
    int camFMT = 0;

    ccalib::CameraParameters camParams;
    camParams.width = 640;
    camParams.height = 480;
    camParams.ratio = 640.0f / 480.0f;
    camParams.exposure = 0.333;
    camParams.fps = 30;
    camParams.autoExposure = false;
    camParams.format = "YUVY";

    // Calibration specific state variables
    ccalib::CheckerboardFrame frame;
    ccalib::Calibrator calib(8, 11, 0.022);
    ccalib::Corners frameCorners;
    vector<cv::Point2f> corners;
    float skewRatio = ((calib.checkerboardCols - 1.0f) / (calib.checkerboardRows - 1.0f));

    // Coverage specific state variables
    ccalib::CoverageParameters coverage;

    // Initialize camera matrix && dist coeff
    ccalib::CalibrationParameters calibParams;
    int snapID = -1;
    vector<ccalib::Snapshot> snapshots;
    vector<float> instance_errs;

    // Initialize Target Frames for automatic collection
    int curr_target = 0;
    vector<vector<cv::Point2f>> target_frames;
    // Big Frame
    target_frames.push_back({cv::Point2f(0.05f, 0.05f), cv::Point2f(0.95f, 0.05f),
                             cv::Point2f(0.95f, 0.95f), cv::Point2f(0.05f, 0.95f)});
    // 4 Medium Frames
    target_frames.push_back({cv::Point2f(0.05f, 0.05f), cv::Point2f(0.55f, 0.05f),
                             cv::Point2f(0.55f, 0.55f), cv::Point2f(0.05f, 0.55f)});
    target_frames.push_back({cv::Point2f(0.45f, 0.05f), cv::Point2f(0.95f, 0.05f),
                             cv::Point2f(0.95f, 0.55f), cv::Point2f(0.45f, 0.55f)});
    target_frames.push_back({cv::Point2f(0.45f, 0.45f), cv::Point2f(0.95f, 0.45f),
                             cv::Point2f(0.95f, 0.95f), cv::Point2f(0.45f, 0.95f)});
    target_frames.push_back({cv::Point2f(0.05f, 0.45f), cv::Point2f(0.55f, 0.45f),
                             cv::Point2f(0.55f, 0.95f), cv::Point2f(0.05f, 0.95f)});
    // 9 Small Frames
    target_frames.push_back({cv::Point2f(0.05f, 0.05f), cv::Point2f(0.35f, 0.05f),
                             cv::Point2f(0.35f, 0.35f), cv::Point2f(0.05f, 0.35f)});
    target_frames.push_back({cv::Point2f(0.35f, 0.05f), cv::Point2f(0.65f, 0.05f),
                             cv::Point2f(0.65f, 0.35f), cv::Point2f(0.35f, 0.35f)});
    target_frames.push_back({cv::Point2f(0.65f, 0.05f), cv::Point2f(0.95f, 0.05f),
                             cv::Point2f(0.95f, 0.35f), cv::Point2f(0.65f, 0.35f)});
    target_frames.push_back({cv::Point2f(0.65f, 0.35f), cv::Point2f(0.95f, 0.35f),
                             cv::Point2f(0.95f, 0.65f), cv::Point2f(0.65f, 0.65f)});
    target_frames.push_back({cv::Point2f(0.35f, 0.35f), cv::Point2f(0.65f, 0.35f),
                             cv::Point2f(0.65f, 0.65f), cv::Point2f(0.35f, 0.65f)});
    target_frames.push_back({cv::Point2f(0.05f, 0.35f), cv::Point2f(0.35f, 0.35f),
                             cv::Point2f(0.35f, 0.65f), cv::Point2f(0.05f, 0.65f)});
    target_frames.push_back({cv::Point2f(0.05f, 0.65f), cv::Point2f(0.35f, 0.65f),
                             cv::Point2f(0.35f, 0.95f), cv::Point2f(0.05f, 0.95f)});
    target_frames.push_back({cv::Point2f(0.35f, 0.65f), cv::Point2f(0.65f, 0.65f),
                             cv::Point2f(0.65f, 0.95f), cv::Point2f(0.35f, 0.95f)});
    target_frames.push_back({cv::Point2f(0.65f, 0.65f), cv::Point2f(0.95f, 0.65f),
                             cv::Point2f(0.95f, 0.95f), cv::Point2f(0.65f, 0.95f)});


    // UI specific variables
    int width_parameter_window = 350;
    float spacing = (width_parameter_window - ImGui::GetStyle().WindowPadding.x * 2) / 2;

    // Camera Formats
    // TODO Use v4l2 VIDIOC_ENUM_FMT to read out all valid formats
    vector<int> camera_fps{5, 10, 15, 20, 30, 50, 60, 100, 120};
    vector<string> camera_fmt{"YUVY", "YUY2", "YU12", "YV12", "RGB3", "BGR3", "Y16 ", "MJPG", "MPEG", "X264", "HEVC"};
    ccalib::ImageInstance img(cv::Size(camParams.width, camParams.height), CV_8UC3);
    ccalib::ImageInstance imgPrev(cv::Size(camParams.width, camParams.height), CV_8UC3);
    GLuint texture;

    // ==========================================
    // Start Initialization
    // ==========================================

    // Get all v4l2 devices
    const fs::path device_dir("/dev");
    vector<string> cameras;

    for (const auto &entry : fs::directory_iterator(device_dir)) {
        if (entry.path().string().find("video") != string::npos)
            cameras.push_back(entry.path());
    }

    // Try to open connection to first camera in device list
    ccalib::Camera cam(cameras[camID], camParams);


    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
//        while (SDL_WaitEventTimeout(&event, 10)) {
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

            ImGui::Begin("Settings", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
            ImGui::PushItemWidth(-1);

            // Organize into 3 tabs (Parameters, Calibration Procedure, Results)
            if (ImGui::BeginTabBar("##tabBar", ImGuiTabBarFlags_None)) {

                // ==========================================
                // Camera Settings and Calibration Parameters
                // ==========================================

                if (ImGui::BeginTabItem("Parameters")) {
                    // Camera Card
                    if (ccalib::BeginCard("Camera", font_title, 4.5f + calibrated, show_camera_card)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Device");
                        ImGui::SameLine(spacing);

                        if (ImGui::BeginCombo("##camera_selector", cameras[camID].c_str(), 0)) {
                            for (int i = 0; i < cameras.size(); i++) {
                                bool is_selected = (cameras[camID] == cameras[i]);
                                if (ImGui::Selectable(cameras[i].c_str(), is_selected)) {
                                    if (cameraOn) {
                                        cam.stopStream();
                                        cam.close();
                                        cameraOn = false;
                                    }
                                    camID = i;
                                }
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Stream");
                        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                        ccalib::ToggleButton("##cam_toggle", &cameraOn, !cameraOn);
                        if (cameraOn && ImGui::IsItemClicked(0)) {
                            cam.open();
                            cam.updateParameters(camParams);
                            cam.startStream();
                            changed = true;
                        } else if (!cameraOn & ImGui::IsItemClicked(0)) {
                            cam.stopStream();
                            cam.close();
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Flip Image");
                        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                        ccalib::ToggleButton("##flip_toggle", &flip_img);

                        if (calibrated) {
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Undistort Image");
                            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                            ccalib::ToggleButton("##undistort_toggle", &undistort);
                        }

                        ccalib::EndCard();
                    }

                    // Camera Parameters Card
                    if (ccalib::BeginCard("Parameters", font_title, 5.5, show_parameters_card)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Resolution");
                        ImGui::SameLine(spacing);
                        ImGui::PushItemWidth(44);
                        ImGui::InputInt("##width", &camParams.width, 0);
                        ImGui::SameLine();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("x");
                        ImGui::SameLine();
                        ImGui::InputInt("##height", &camParams.height, 0);
                        ImGui::PopItemWidth();
                        const char *button_text = "Set";
                        ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(button_text).x -
                                        style.FramePadding.x);
                        if (ccalib::MaterialButton(button_text)) {
                            cam.updateResolution(camParams.width, camParams.height);
                            changed = true;
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Framerate");
                        ImGui::SameLine(spacing);
                        if (ImGui::BeginCombo("##camera_fps", to_string(camera_fps[camFPS]).c_str(), 0)) {
                            for (int i = 0; i < camera_fps.size(); i++) {
                                bool is_selected = (camera_fps[camFPS] == camera_fps[i]);
                                if (ImGui::Selectable(to_string(camera_fps[i]).c_str(), is_selected)) {
                                    camFPS = i;
                                    if (!ImGui::IsMouseClicked(0)) {
                                        camParams.fps = (int) camera_fps[camFPS];
                                        cam.updateFramerate(camParams.fps);
                                    }
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                            }
                            ImGui::EndCombo();
                            changed = true;
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Exposure Time");
                        ImGui::SameLine(spacing);
                        if (ImGui::SliderFloat("##camera_exptime", &camParams.exposure, 0, 1, "%.3f", 2.0)) {
                            cam.updateExposure(camParams.exposure);
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Select Format");
                        ImGui::SameLine(spacing);
                        if (ImGui::BeginCombo("##camera_fmt", camera_fmt[camFMT].c_str(), 0)) {
                            for (int i = 0; i < camera_fmt.size(); i++) {
                                bool is_selected = (camera_fmt[camFMT] == camera_fmt[i]);
                                if (ImGui::Selectable(camera_fmt[i].c_str(), is_selected)) {
                                    camFMT = i;
                                    if (!ImGui::IsMouseClicked(0)) {
                                        camParams.format = camera_fmt[camFMT];
                                        cam.updateFormat(camParams.format);
                                    }
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                            }
                            ImGui::EndCombo();
                            changed = true;
                        }


                        ccalib::EndCard();
                    }

                    // Calibration Parameters Card
                    if (ccalib::BeginCard("Calibration", font_title, 5.5, show_calibration_card)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Rows");
                        ImGui::SameLine(spacing);
                        ImGui::InputInt("##chkbrd_rows", &calib.checkerboardRows, 1);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Cols");
                        ImGui::SameLine(spacing);
                        ImGui::InputInt("##chkbrd_cols", &calib.checkerboardCols, 1);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Size in [m]");
                        ImGui::SameLine(spacing);
                        ImGui::InputFloat("##chkbrd_size", &calib.checkerboardSize, 0.001f);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Calibration");
                        const char *button_text = calibration_mode ? "Reset" : "Start";
                        ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(button_text).x -
                                        style.FramePadding.x);
                        if (ccalib::MaterialButton(button_text, !calibration_mode && cam.isStreaming())) {
                            calibration_mode = !calibration_mode;
                            if (cameraOn && calibration_mode) {
                                ccalib::CalibrationParameters newCalibParams;
                                ccalib::CoverageParameters newCoverage;
                                ccalib::CheckerboardFrame newFrame;
                                coverage = newCoverage;
                                frame = newFrame;
                                calibParams = newCalibParams;
                                undistort = false;
                                snapID = -1;

                                snapshots.clear();
                                instance_errs.clear();
                            }
                        }

                        if (!cameraOn)
                            calibration_mode = false;

                        ccalib::EndCard();
                    }
                    ImGui::EndTabItem();
                }

                if (changed) {
                    // Update params
                    camParams = cam.getParameters();
                    for (int i = 0; i < camera_fps.size(); i++)
                        camFPS = (camera_fps[i] == camParams.fps) ? i : camFPS;
                    changed = false;
                }

                if (cam.isStreaming()) {
                    if (cam.getFrameCount() != imgPrev.id)
                        img.id = cam.captureFrame(img.data);
                    else
                        img = imgPrev;
                }

                // ==========================================
                // Calibration Snapshots
                // ==========================================

                if (calibration_mode) {
                    // TODO create calibration class to hold all related variables and methods
                    if (ImGui::BeginTabItem("Calibration")) {

                        // Detect Checkerboard
                        // TODO Only run if image has changed
                        if (cam.isStreaming() && img.id != imgPrev.id) {
                            cv::Mat gray(img.data.rows, img.data.cols, CV_8UC1);
                            cv::cvtColor(img.data, gray, cv::COLOR_RGB2GRAY);
                            cv::cvtColor(gray, img.data, cv::COLOR_GRAY2RGB);

                            // TODO optimize procedure, only look for checkerboard in previous area + delta
                            img.hasCheckerboard = calib.findCorners(gray, corners);

//                            if (corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1))) {
                            if (img.hasCheckerboard) {
                                ccalib::Corners fc({corners[0], corners[calib.checkerboardCols - 2], corners[corners.size() - 1],
                                                    corners[corners.size() - calib.checkerboardCols + 1]});
                                ccalib::absToRelativePoints(fc.points, cv::Size(camParams.width, camParams.height));
                                double width = max(cv::norm(fc.topRight() - fc.topLeft()),
                                                   cv::norm(fc.bottomRight() - fc.bottomLeft()));
                                double height = max(cv::norm(fc.bottomLeft() - fc.topLeft()),
                                                    cv::norm(fc.bottomRight() - fc.topRight()));
                                frame.pos = fc.topLeft() + (fc.bottomRight() - fc.topLeft()) / 2;
                                frame.size = (float) sqrt(width * height);
                                frame.skew = (float) log(width / height / skewRatio * camParams.ratio) / 3.0f + 0.5f;
                                frameCorners = fc;
                            }
                        }

                        // Show Coverage Card
                        if (ccalib::BeginCard("Coverage", font_title, 9.5, show_coverage_card)) {
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Horizontal Coverage");

                            ccalib::CoveredBar(coverage.x_min - 0.1f, coverage.x_max + 0.1f, 1.0f - frame.pos.x);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Vertical Coverage");

                            ccalib::CoveredBar(coverage.y_min - 0.1f, coverage.y_max + 0.1f, 1.0f - frame.pos.y);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Size Coverage");
                            ccalib::CoveredBar(coverage.size_min - 0.1f, coverage.size_max + 0.1f, frame.size);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Skew Coverage");
                            ccalib::CoveredBar(coverage.skew_min - 0.1f, coverage.skew_max + 0.1f, frame.skew);

                            ccalib::EndCard();
                        }

                        // Snapshots Card
                        if (ccalib::BeginCard("Snapshots", font_title, 3.3f + snapshots.size() * 0.76f,
                                              show_snapshots_card)) {
                            // Collect snapshot button
                            // TODO automatic collection of snapshots
                            const char *status_text;
                            if (ccalib::MaterialButton("Snapshot", !calibrated && snapshots.size() < 4) &&
                                cam.isStreaming())
                                taking_snapshot = true;

                            if (taking_snapshot)
                                status_text = "Try not to move...";
                            else if (!corners.empty())
                                status_text = "Ready";
                            else
                                status_text = "No Checkerboard detected!";
                            float movement = 0.0f;

//                            if (cam.isStreaming() && corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1))) {
                            if (cam.isStreaming() && img.hasCheckerboard) {
                                // Compare actual frame with previous frame for movement
                                cv::Rect rect = cv::minAreaRect(corners).boundingRect();
                                cv::Mat oldImg;
                                cv::Mat newImg;
                                if (rect.x > 0 && rect.y > 0 && rect.x + rect.width < camParams.width &&
                                    rect.y + rect.height < camParams.height) {
                                    oldImg = imgPrev.data(rect);
                                    newImg = img.data(rect);
                                } else {
                                    imgPrev.data.copyTo(oldImg);
                                    img.data.copyTo(newImg);
                                }
                                cv::cvtColor(oldImg, oldImg, cv::COLOR_RGB2GRAY);
                                cv::cvtColor(newImg, newImg, cv::COLOR_RGB2GRAY);
                                cv::Mat diff(oldImg.rows, oldImg.cols, CV_8UC1);
                                cv::absdiff(oldImg, newImg, diff);
                                cv::Scalar mean_diff = cv::mean(diff);
                                movement = 1.0f - (float) mean_diff.val[0] / 127.0f;

                                // If successful, add instance
                                if (taking_snapshot && mean_diff.val[0] < 4) {
                                    ccalib::Snapshot instance;
                                    img.data.copyTo(instance.img.data);
                                    instance.img.id = img.id;
                                    instance.corners.assign(corners.begin(), corners.end());
                                    instance.frame = frame;
                                    instance.frameCorners.points.assign(frameCorners.points.begin(),
                                                                        frameCorners.points.end());
                                    snapshots.push_back(instance);

                                    // Update coverage
                                    ccalib::updateCoverage(snapshots, coverage);

                                    if (snapshots.size() >= 4) {
                                        calib.calibrateCameraBG(snapshots, calibParams, instance_errs);
                                        calibrated = calibParams.reprojErr < 0.3f;
                                        undistort = true;
                                    }

                                    taking_snapshot = false;
                                    if (in_target)
                                        curr_target++;
                                }
                            }

                            ImGui::SameLine();
                            ImGui::Text("%s", status_text);
                            ccalib::CoveredBar(0.0f, movement);

                            // List all snapshots
                            // TODO progress bar of how many snapshots need to be taken
                            if (!snapshots.empty()) {
                                // List all snapshots
                                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - 16);
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 11));
                                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));
                                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
                                ImGui::BeginColumns("##snapshots", 1, ImGuiColumnsFlags_NoBorder);
                                ImGui::BeginGroup();
                                ImDrawList *drawList = ImGui::GetWindowDrawList();
                                for (int i = 0; i < snapshots.size(); i++) {
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                                    bool is_selected = i == snapID;
                                    string text;
                                    int stamp = snapshots[i].img.id;
                                    text = to_string(i) + " FrameID: " + to_string(stamp);
                                    ImVec2 p = ImGui::GetCursorScreenPos();
                                    ImVec2 s(ImGui::GetContentRegionAvailWidth(),
                                             ImGui::GetTextLineHeight() + 4);

                                    ImVec4 color;
                                    if (instance_errs.size() > i) {
                                        color = ccalib::interp_color(instance_errs[i], 0.0f, 1.0f);
                                        if (i != snapID)
                                            drawList->AddRectFilled(ImVec2(p.x - 4, p.y - 4),
                                                                    ImVec2(p.x + s.x + 4, p.y + s.y),
                                                                    ImGui::GetColorU32(
                                                                            ImVec4(color.x, color.y, color.z, 0.5f)),
                                                                    3.0f);
                                    } else
                                        color = ImVec4(0.56f, 0.83f, 0.26f, 1.0f);

                                    if (ImGui::Selectable(text.c_str(), is_selected))
                                        snapID = is_selected ? -1 : i;

                                    if (ImGui::IsItemHovered())
                                        drawList->AddRect(ImVec2(p.x - 5, p.y - 4),
                                                          ImVec2(p.x + s.x + 5, p.y + s.y),
                                                          ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 0.75f)),
                                                          3.0f, ImDrawCornerFlags_All, 2.0f);

                                    if (is_selected) {
                                        drawList->AddRect(ImVec2(p.x - 5, p.y - 4),
                                                          ImVec2(p.x + s.x + 5, p.y + s.y),
                                                          ImGui::GetColorU32(color),
                                                          3.0f, ImDrawCornerFlags_All, 2.0f);
                                        if (ImGui::BeginPopupContextItem()) {
                                            if (ImGui::Selectable("Remove")) {
                                                snapshots.erase(snapshots.begin() + snapID);

                                                // Update coverage
                                                ccalib::updateCoverage(snapshots, coverage);

                                                snapID--;
                                                if (snapshots.size() >= 4) {
                                                    calib.calibrateCameraBG(snapshots, calibParams, instance_errs);
                                                    calibrated = calibParams.reprojErr < 0.3f;
                                                }
                                            }
                                            ImGui::EndPopup();
                                        }
                                    }
                                }
                                ImGui::EndGroup();
                                ImGui::EndColumns();
                                ImGui::PopStyleColor(2);
                                ImGui::PopStyleVar(1);
                                ImGui::PopItemWidth();
                            }
                            ccalib::EndCard();
                        }
                        ImGui::EndTabItem();
                    } else {
                        corners.clear();
                        frameCorners.points.clear();
                    }
                }

                // ==========================================
                // Results Tab
                // ==========================================

                if (snapshots.size() >= 4 && calibration_mode && calibrated) {
                    if (ImGui::BeginTabItem("Results")) {
                        // Results Card
                        if (ccalib::BeginCard("Results", font_title, 7.5,
                                              show_result_card)) {
                            if (ccalib::MaterialButton("Re-Calibrate", false) ||
                                snapshots.size() != instance_errs.size()) {
                                calib.calibrateCameraBG(snapshots, calibParams, instance_errs);
                                calibrated = calibParams.reprojErr < 0.3f;
                                undistort = true;
                            }

                            if (calibrated) {
                                ImGui::SameLine();
                                if (ccalib::MaterialButton("Export", calibrated)) {
                                    cv::FileStorage fs("calibration.yaml",
                                                       cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);
                                    fs << "image_width" << camParams.width;
                                    fs << "image_height" << camParams.height;
                                    fs << "camera_name" << cameras[camID];
                                    fs << "camera_matrix" << calibParams.K;
                                    fs << "distortion_model" << "plumb_bob";
                                    fs << "distortion_coefficients" << calibParams.D;
                                    fs << "rectification_matrix" << cv::Mat::eye(3, 3, CV_64F);
                                    cv::Mat P;
                                    cv::hconcat(calibParams.K, cv::Mat::zeros(3, 1, CV_64F), P);
                                    fs << "projection_matrix" << P;
                                    fs.release();

                                    ImGui::SameLine();
                                    ImGui::Text("Exported!");
                                }
                                stringstream result_ss;
                                result_ss << "K = " << calibParams.K << endl << endl;
                                result_ss << "D = " << calibParams.D;
                                string result = result_ss.str();
                                char output[result.size() + 1];
                                strcpy(output, result.c_str());
                                ImGui::InputTextMultiline("##result", output, result.size(),
                                                          ImVec2(0, ImGui::GetTextLineHeight() * 11),
                                                          ImGuiInputTextFlags_ReadOnly);
                            }
                            ccalib::EndCard();
                        }
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();

                if (snapID != -1) {
                    cam.stopStream();
                    img = snapshots[snapID].img;
                    corners = snapshots[snapID].corners;
                    frame = snapshots[snapID].frame;
                    frameCorners = snapshots[snapID].frameCorners;
                } else if (cam.isOpened() && !cam.isStreaming()) {
                    cam.startStream();
                    img.id = cam.captureFrame(img.data);
                }
            }

            imgPrev = img;

            ImGui::End();
        }


        // Show image preview
        {
            // Set next window size & pos
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::SetNextWindowPos(ImVec2(width_parameter_window, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - width_parameter_window, io.DisplaySize.y),
                                     ImGuiCond_Always);

            ImGui::Begin("Preview", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoTitleBar);

            if (cameraOn) {
                if (snapID == -1 && img.id != imgPrev.id) {
                    if (flip_img)
                        cv::flip(img.data, img.data, 1);
                    if (undistort) {
                        cv::Mat undistorted;
                        cv::undistort(img.data, undistorted, calibParams.K, calibParams.D);
                        undistorted.copyTo(img.data);
                    }
                }
                glDeleteTextures(1, &texture);
                ccalib::mat2Texture(img.data, texture);
            }

            // Camera image
            float width_avail = ImGui::GetContentRegionAvail().x;
            float height_avail = ImGui::GetContentRegionAvail().y;
            float scaling = 1.0f;
            cv::Size img_size_old(img.data.cols, img.data.rows);
            if (!img.data.empty()) {
                if (height_avail * camParams.ratio > width_avail) {
                    scaling = width_avail / img.data.cols;
                    cv::resize(img.data, img.data, cv::Size((int) width_avail, (int) (width_avail / camParams.ratio)));
                } else {
                    scaling = height_avail / img.data.rows;
                    cv::resize(img.data, img.data,
                               cv::Size((int) (height_avail * camParams.ratio), (int) height_avail));
                }
            }

            // Positioning && Centering
            ImVec2 pos = ImVec2((width_avail - img.data.cols) / 2 + ImGui::GetCursorPosX(),
                                (height_avail - img.data.rows) / 2 + ImGui::GetCursorPosY());
            ImGui::SetCursorPos(pos);
            ImGui::Image((void *) (intptr_t) texture, ImVec2(img.data.cols, img.data.rows));
            cv::Point2f offset(pos.x + width_parameter_window, pos.y);

            // Draw Corners
            if (calibration_mode && !corners.empty()) {
                vector<cv::Point2f> drawCorners(corners);
                if (flip_img && snapID == -1) {
                    ccalib::flipPoints(drawCorners, img_size_old);
                }
                for (auto &p : drawCorners) {
                    p *= scaling;
                    p += offset;
                }
                ccalib::drawPoints(drawCorners, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), frame.size * 4.0f);
            }

            // Draw Frame around checkerboard
            ccalib::Corners drawFrameCorners = frameCorners;
            if (calibration_mode && !corners.empty() && !drawFrameCorners.points.empty()) {
                ccalib::relativeToAbsPoints(drawFrameCorners.points, img_size_old);
                ccalib::increaseRectSize(drawFrameCorners.points, frame.size * 64);
                if (flip_img) {
                    ccalib::flipPoints(drawFrameCorners.points, img_size_old);
                }

                // Convert to img coordinates
                for (auto &p : drawFrameCorners.points) {
                    p *= scaling;
                    p += offset;
                }
                if (curr_target >= target_frames.size())
                    ccalib::drawRectangle(drawFrameCorners.points, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), 4.0f, false);
            }

            // Draw target frames
            if (calibration_mode && curr_target < target_frames.size()) {
                // Convert and flip points
                vector<cv::Point2f> target_corners(target_frames[curr_target]);
                if (flip_img) {
                    ccalib::flipPoints(target_corners, cv::Size(1, 1));
                }

                float checkerboardRatio = (float) calib.checkerboardCols / (float) calib.checkerboardRows;
                float ratioOffset = (img.data.rows * camParams.ratio - img.data.rows * checkerboardRatio) / 2.0f;
                for (int i = 0; i < drawFrameCorners.points.size(); i++) {
                    target_corners[i].x = target_corners[i].x * img.data.rows * checkerboardRatio + ratioOffset + offset.x;
                    target_corners[i].y = target_corners[i].y * img.data.rows + offset.y;
                }

                if (!corners.empty() && !drawFrameCorners.points.empty() && snapID == -1) {
                    double dist = cv::norm((target_corners[0] + (target_corners[2] - target_corners[0]) / 2) -
                                           (drawFrameCorners.topLeft() +
                                            (drawFrameCorners.bottomRight() - drawFrameCorners.topLeft()) / 2));
                    double frameArea = cv::contourArea(drawFrameCorners.points);
                    double targetArea = cv::contourArea(target_corners);

                    ImVec4 col_bg = ccalib::interp_color((float) dist, 0, img.data.rows / 2.0f);

                    if (dist <= frame.size * 64 * scaling && frameArea > targetArea * 0.8f &&
                        frameArea < targetArea * 1.2f) {
                        if (!taking_snapshot) {
                            taking_snapshot = true;
                        }
                        in_target = true;
                        ccalib::drawRectangle(target_corners, ImVec4(0.13f, 0.83f, 0.91f, 1.00f), 24.0f, true);
                    } else {
                        taking_snapshot = false;
                        in_target = false;
                        ccalib::drawRectangle(target_corners, col_bg, 12.0f, true);
                    }

                    double area_diff = abs(1.0f - (float) frameArea / (float) targetArea);
                    col_bg = ccalib::interp_color((float) area_diff, 0, 1.0f);
                    ccalib::drawRectangle(drawFrameCorners.points, col_bg, 4.0f, true);
                }
            }

            if (calibParams.reprojErr != DBL_MAX) {
                string reproj_error;
                if (snapID == -1)
                    reproj_error = "Mean Reprojection Error: " + to_string(calibParams.reprojErr);
                else
                    reproj_error = "Reprojection Error: " + to_string(instance_errs[snapID]);
                float text_width = ImGui::CalcTextSize(reproj_error.c_str()).x;
                ImGui::SetCursorPos(ImVec2(pos.x + img.data.cols - text_width - 16, pos.y + 17));
                ImGui::TextColored(ImColor(0, 0, 0, 255), "%s", reproj_error.c_str());
                ImGui::SetCursorPos(ImVec2(pos.x + img.data.cols - text_width - 16, pos.y + 16));
                ImGui::TextColored(ImColor(255, 255, 255, 255), "%s", reproj_error.c_str());
            }

            ImGui::End();
            ImGui::PopStyleColor(1);
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
