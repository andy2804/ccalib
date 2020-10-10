//
// Created by andy2804 on 19.12.19.
//

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "camera.h"
#include "calibrator.h"
#include "functions.h"
#include "imgui_extensions.h"
#include "imgui_widgets.h"

#include <SDL.h>
#include <experimental/filesystem>
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
    (void) io;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    ccalib::StyleColorsMaterial();
    ImGuiStyle style = ImGui::GetStyle();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    ImFont *fontText = io.Fonts->AddFontFromFileTTF("../resources/Roboto-Regular.ttf", 16.0f);
    ImFont *fontTitle = io.Fonts->AddFontFromFileTTF("../resources/Roboto-Medium.ttf", 24.0f);

    // Initialize state variables
    ccalib::GUIStateVariables state;
    state.fontTitle = fontTitle;
    state.widthParameterWindow = 350;
    state.widthItemSpacing = (state.widthParameterWindow - ImGui::GetStyle().WindowPadding.x * 2) / 2;
    state.fpsID = 4;

    // UI specific variables
    int frameCount = 0;
    int frameLastAction = 0;
    int widthParameterWindow = 350;
    float spacing = (widthParameterWindow - ImGui::GetStyle().WindowPadding.x * 2) / 2;
    string loadingSequence = "/-\\|";

    // General State variables
    // TODO create stateVariables struct
    // TODO pack all Cards into single "widgets"
    bool showCamera = true;
    bool showCamParameters = false;
    bool showCalParameters = false;
    bool showCalibration = true;
    bool showCoverage = true;
    bool showSnapshots = true;
    bool showResults = true;
    bool camParamsChanged = false;
    bool frameChanged = false;
    bool cameraOn = false;
    bool flipImg = false;
    bool undistort = false;
    bool calibrated = false;
    bool takeSnapshot = false;
    bool inTarget = false;
    bool initialized = false;

    // Camera specific state variables
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
    string statusText;
    ccalib::CheckerboardFrame frame;
    ccalib::Calibrator calib(8, 11, 0.022);
    ccalib::Corners frameCorners;
    ccalib::Corners initialCorners({cv::Point2f(0.25, 0.25), cv::Point2f(0.75, 0.25),
                                    cv::Point2f(0.75, 0.75), cv::Point2f(0.25, 0.75)});

    ccalib::CoverageParameters coverage;
    ccalib::CalibrationParameters calibParams;
    vector<ccalib::Snapshot> snapshots;
    vector<cv::Point2f> corners;
    vector<double> instanceErrs;
    float imageMovement = 0.0f;
    int snapID = -1;
    float snapshotDensity = 0.06f;

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

    state.cameras = cameras;

    // Try to open connection to first camera in device list
    ccalib::Camera cam((cameras.size()) ? cameras[camID] : "", camParams);

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

        // ==========================================
        // UI CONTROLS
        // ==========================================

        // Set next window size & pos
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(widthParameterWindow, io.DisplaySize.y), ImGuiCond_Always);

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
//                ccalib::CameraCard(state, cam, camParams);
                if (ccalib::BeginCard("Camera", fontTitle, 4.5f + calibrated, showCamera)) {
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
                        camParamsChanged = true;
                    } else if (!cameraOn & ImGui::IsItemClicked(0)) {
                        cam.stopStream();
                        cam.close();
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Flip Image");
                    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                    ccalib::ToggleButton("##flip_toggle", &flipImg);

                    if (calibrated) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Undistort Image");
                        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                        ccalib::ToggleButton("##undistort_toggle", &undistort);
                    }

                    ccalib::EndCard();
                }

                // Camera Parameters Card
//                ccalib::CameraParametersCard(state, cam, camParams);
                if (ccalib::BeginCard("Camera Parameters", fontTitle, 5.5, showCamParameters)) {
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
                        camParamsChanged = true;
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
                        camParamsChanged = true;
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
                        camParamsChanged = true;
                    }
                    ccalib::EndCard();
                }

                // Calibration Parameters Card
                if (ccalib::BeginCard("Calibration Parameters", fontTitle, 4.5, showCalParameters)) {
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

                    ccalib::EndCard();
                }
                ImGui::EndTabItem();
            }

            if (camParamsChanged) {
                // Update params
                camParams = cam.getParameters();
                for (int i = 0; i < camera_fps.size(); i++)
                    camFPS = (camera_fps[i] == camParams.fps) ? i : camFPS;
                camParamsChanged = false;
            }

            if (cam.isStreaming()) {
                if (cam.getFrameCount() != imgPrev.id) {
                    img.id = cam.captureFrame(img.data);
                    img.hasCheckerboard = false;
                    frameChanged = true;
                } else {
                    img = imgPrev;
                    frameChanged = false;
                }
            } else
                frameChanged = false;

            // ==========================================
            // Calibration Snapshots
            // ==========================================

            if (cam.isOpened() && ImGui::BeginTabItem("Calibration")) {

                // Detect Checkerboard
                if (cam.isStreaming() && frameChanged) {
                    cv::Mat gray(img.data.rows, img.data.cols, CV_8UC1);
                    cv::cvtColor(img.data, gray, cv::COLOR_RGB2GRAY);
                    cv::cvtColor(gray, img.data, cv::COLOR_GRAY2RGB);
                    cv::normalize(gray, gray, 255, 0, cv::NORM_MINMAX);

                    // Use prior to clip to ROI
                    cv::Rect prior = cv::Rect(0, 0, img.data.cols, img.data.rows);
                    double priorScale = 1.0f;
                    if (imgPrev.hasCheckerboard) {
                        ccalib::Corners priorFrameCorners = frameCorners;
                        ccalib::relativeToAbsPoints(priorFrameCorners.points,
                                                    cv::Size(img.data.cols, img.data.rows));
                        ccalib::increaseRectSize(priorFrameCorners.points, frame.size * img.data.cols * 0.2f);
                        prior = prior & cv::boundingRect(priorFrameCorners.points);
                        gray = gray(prior);
                        cv::normalize(gray, gray, 255, 0, cv::NORM_MINMAX);
                        priorScale = min(1.0f, gray.cols / 480.0f);
                        float previousWidth = gray.cols;
                        cv::resize(gray, gray, cv::Size(int(gray.cols / priorScale), int(gray.rows / priorScale)), 0, 0,
                                   cv::INTER_LINEAR_EXACT);
                        priorScale = previousWidth / gray.cols;
                    }

                    // Find corners
                    if ((img.hasCheckerboard = calib.findCorners(gray, corners))) {
                        // Add prior offset
                        if (imgPrev.hasCheckerboard)
                            for (auto &p : corners) {
                                p *= priorScale;
                                p.x += prior.x;
                                p.y += prior.y;
                            }
                        calib.computeFrame(corners, camParams, frame, frameCorners);
                    } else
                        frameCorners.points.clear();
                }

                // Show Coverage Card
                if (ccalib::BeginCard("Calibration", fontTitle, 2.3, showCalibration)) {
                    if (!initialized)
                        statusText = "Waiting for initialization... ";
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", statusText.c_str());
                    ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 44);

                    ccalib::MaterialCancelButton("Reset", false);
                    if (ImGui::IsItemClicked()) {
                        // Reset all parameters
                        ccalib::CalibrationParameters newCalibParams;
                        ccalib::CoverageParameters newCoverage;
                        ccalib::CheckerboardFrame newFrame;
                        coverage = newCoverage;
                        frame = newFrame;
                        calibParams = newCalibParams;
                        undistort = false;
                        initialized = false;
                        calibrated = false;
                        snapID = -1;
                        snapshots.clear();
                        instanceErrs.clear();
                    }

                    ccalib::EndCard();
                }

                // Show Coverage Card
                if (ccalib::BeginCard("Coverage", fontTitle, 9.5, showCoverage)) {
                    float highlight = !initialized ? 0.5f : -1;
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Horizontal Coverage");
                    ccalib::CoveredBar(coverage.x_min - 0.1f, coverage.x_max + 0.1f, 1.0f - frame.pos.x, highlight);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertical Coverage");
                    ccalib::CoveredBar(coverage.y_min - 0.1f, coverage.y_max + 0.1f, 1.0f - frame.pos.y, highlight);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Size Coverage");
                    ccalib::CoveredBar(coverage.size_min - 0.1f, coverage.size_max + 0.1f, frame.size, highlight);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Skew Coverage");
                    ccalib::CoveredBar(coverage.skew_min - 0.1f, coverage.skew_max + 0.1f, frame.skew, highlight);
                    ccalib::EndCard();
                }

                // Snapshots Card
                if (ccalib::BeginCard("Snapshots", fontTitle, 2.7f + snapshots.size() * 0.78f, showSnapshots)) {
                    // Collect snapshot button
                    if (ccalib::MaterialButton("Snapshot", !calibrated && initialized, initialized) && cam.isStreaming())
                        takeSnapshot = true;

                    // Check if view is different enough
                    if (!initialized) {
                        if (ccalib::checkFrameInTarget(frame, *new ccalib::CheckerboardFrame()))
                            takeSnapshot = true;
                        else
                            takeSnapshot = false;
                    }
                    if (initialized && ccalib::checkCoverage(coverage, frame, snapshotDensity))
                        takeSnapshot = true;

                    if (takeSnapshot && img.hasCheckerboard)
                        statusText = "Capturing, don't move... ";
                    else if (img.hasCheckerboard)
                        statusText = "Ready ";
                    else {
                        statusText = "No Checkerboard! ";
                        imageMovement = 0.0f;
                    }

                    // Compare actual frame with previous frame for movement
                    if (frameChanged && img.hasCheckerboard) {
                        cv::Rect rect = cv::minAreaRect(corners).boundingRect();
                        imageMovement = ccalib::computeImageDiff(img.data, imgPrev.data, rect);
                    }

                    // If successful, add instance
                    if (takeSnapshot && imageMovement > 0.97f) {
                        inTarget = true;
                        frameLastAction = frameCount;

                        // Save snapshot
                        ccalib::Snapshot instance;
                        instance.img.data = img.data.clone();
                        instance.img.id = img.id;
                        instance.corners = corners;
                        instance.frame = frame;
                        instance.frameCorners = frameCorners;
                        snapshots.push_back(instance);

                        // Update coverage & calibration
                        ccalib::updateCoverage(snapshots, coverage);
                        takeSnapshot = false;
                    }

                    // Update calibration parameters
                    if (snapshots.size() >= 4 && snapshots.size() != instanceErrs.size() && !calib.isCalibrating()) {
                        if (snapshots.size() >= 4 && !calib.isCalibrating()) {
                            calib.calibrateCameraBG(snapshots, calibParams, instanceErrs);
                            calibrated = calibParams.reprojErr <= 0.3f && calibParams.reprojErrVar <= 0.1f;
                            undistort = true;
                        }
                    } else if (calib.isCalibrating())
                        statusText = "Calibrating... ";

                    if (takeSnapshot || calib.isCalibrating() || !initialized)
                        statusText += loadingSequence[frameCount % 8 / 2];

                    ImGui::SameLine();
                    ccalib::CoveredBar(0.0f, imageMovement);

                    // List all snapshots
                    // TODO progress bar of how many snapshots need to be taken
                    if (!snapshots.empty()) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
                        // List all snapshots
                        if (!initialized)
                            initialized = true;
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - 16);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
                        ImGui::BeginColumns("##snapshots", 1, ImGuiColumnsFlags_NoBorder);
                        ImGui::BeginGroup();
                        ImVec2 size(ImGui::GetContentRegionAvailWidth() - style.FramePadding.x - 24,
                                    ImGui::GetTextLineHeight() + 4);
                        bool anyActiveSnaps = false;
                        for (int i = 0; i < snapshots.size(); i++) {
                            string text = to_string(i) + "  FrameID: " + to_string(snapshots[i].img.id);
                            ImVec4 color;
                            string toolTip;
                            bool deleteAdvice = false;

                            // Check specific reprojection error
                            if (instanceErrs.size() > i) {
                                toolTip = "Error: " + to_string(instanceErrs[i]);
                                color = ccalib::interp_color(instanceErrs[i], 0.0f, 1.0f);
                                deleteAdvice = instanceErrs[i] > 0.5;
                            } else
                                color = ImVec4(0.56f, 0.83f, 0.26f, 1.0f);

                            // Hoverables
                            if (ccalib::Hoverable(text, toolTip, color, size)) {
                                snapID = i;
                                anyActiveSnaps = true;
                            }
                            ImGui::SameLine();
                            if (ccalib::HoverableDeleteButton(text, ImVec2(24, size.y + 4), deleteAdvice)) {
                                // Update Snapshots & Coverage
                                snapshots.erase(snapshots.begin() + i);
                                snapID = -1;
                                ccalib::updateCoverage(snapshots, coverage);
                                if (snapshots.size() < 4) {
                                    instanceErrs.clear();
                                    calibrated = false;
                                }
                                if (snapshots.empty())
                                    initialized = false;
                            }
                        }
                        snapID = anyActiveSnaps ? snapID : -1;
                        ImGui::EndGroup();
                        ImGui::EndColumns();
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

            // ==========================================
            // Results Tab
            // ==========================================

            if (calibrated && ImGui::BeginTabItem("Results")) {
                // Results Card
                if (ccalib::BeginCard("Results", fontTitle, 7.5, showResults)) {
                    if (ccalib::MaterialButton("Export", calibrated)) {
                        frameLastAction = frameCount;
                        cv::FileStorage file("calibration.yaml", cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);
                        file << "image_width" << camParams.width;
                        file << "image_height" << camParams.height;
                        file << "camera_name" << cameras[camID];
                        file << "camera_matrix" << calibParams.K;
                        file << "distortion_model" << "plumb_bob";
                        file << "distortion_coefficients" << calibParams.D;
                        file << "rectification_matrix" << cv::Mat::eye(3, 3, CV_64F);
                        cv::Mat P;
                        cv::hconcat(calibParams.K, cv::Mat::zeros(3, 1, CV_64F), P);
                        file << "projection_matrix" << P;
                        file.release();
                    }
                    if (frameCount - frameLastAction < 60) {
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
                    ccalib::EndCard();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

            if (snapID != -1) {
                cam.stopStream();
                img = snapshots[snapID].img;
                img.hasCheckerboard = true;
                img.data = snapshots[snapID].img.data.clone();
                corners = snapshots[snapID].corners;
                frame = snapshots[snapID].frame;
                frameCorners = snapshots[snapID].frameCorners;
                frameChanged = true;
                takeSnapshot = false;
            } else if (cam.isOpened() && !cam.isStreaming()) {
                cam.startStream();
                img.id = cam.captureFrame(img.data);
            } else {
                imgPrev = img;
                imgPrev.data = img.data.clone();
            }
        }

        ImGui::End();

        // ==========================================
        // IMAGE PREVIEW
        // ==========================================

        // Set next window size & pos
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::SetNextWindowPos(ImVec2(widthParameterWindow, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - widthParameterWindow, io.DisplaySize.y),
                                 ImGuiCond_Always);

        ImGui::Begin("Preview", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoTitleBar);

        // Image to texture
        cv::Mat preview = img.data.clone();
        if (cameraOn) {
            if (snapID == -1) {
                if (undistort)
                    cv::undistort(img.data, preview, calibParams.K, calibParams.D);
                if (flipImg)
                    cv::flip(preview, preview, 1);
            }
            if (frameChanged) {
                glDeleteTextures(1, &texture);
                ccalib::mat2Texture(preview, texture);
            }
        }

        // Resize Camera image
        float widthAvail = ImGui::GetContentRegionAvail().x;
        float heightAvail = ImGui::GetContentRegionAvail().y;
        float scaling = 1.0f;
        cv::Size imgSizeOld(preview.cols, preview.rows);
        if (!preview.empty()) {
            if (heightAvail * camParams.ratio > widthAvail) {
                scaling = widthAvail / preview.cols;
                cv::resize(preview, preview, cv::Size((int) widthAvail, (int) (widthAvail / camParams.ratio)));
            } else {
                scaling = heightAvail / preview.rows;
                cv::resize(preview, preview,
                           cv::Size((int) (heightAvail * camParams.ratio), (int) heightAvail));
            }
        }

        // Positioning && Centering
        ImVec2 pos = ImVec2((widthAvail - preview.cols) / 2 + ImGui::GetCursorPosX(),
                            (heightAvail - preview.rows) / 2 + ImGui::GetCursorPosY());
        ImGui::SetCursorPos(pos);
        ImGui::Image((void *) (intptr_t) texture, ImVec2(preview.cols, preview.rows));
        cv::Point2f offset(pos.x + widthParameterWindow, pos.y);

        // Draw Corners & Frame
        if (!frameCorners.points.empty()) {
            // Draw initial Frame
            if (!initialized) {
                ccalib::Corners temp_ic = initialCorners;
                ccalib::relativeToAbsPoints(temp_ic.points, imgSizeOld);
                ccalib::increaseRectSize(temp_ic.points, 0.5f * img.data.cols * 0.1f);
                for (auto &p : temp_ic.points) {
                    p *= scaling;
                    p += offset;
                }
                ccalib::drawRectangle(temp_ic.points, ImVec4(0.91f, 0.83f, 0.26f, 1.00f), 8.0f, true);
            }

            ccalib::Corners temp_fc = frameCorners;
            vector<cv::Point2f> temp_c(corners);
            ccalib::relativeToAbsPoints(temp_fc.points, imgSizeOld);
            ccalib::increaseRectSize(temp_fc.points, frame.size * img.data.cols * 0.1f);
            if (flipImg && snapID == -1) {
                ccalib::flipPoints(temp_c, imgSizeOld);
                ccalib::flipPoints(temp_fc.points, imgSizeOld);
            }

            // Convert to img coordinates
            for (auto &p : temp_c) {
                p *= scaling;
                p += offset;
            }
            for (auto &p : temp_fc.points) {
                p *= scaling;
                p += offset;
            }

            // Draw
            ccalib::drawPoints(temp_c, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), frame.size * 4.0f);
            if (inTarget) {
                ccalib::drawRectangle(temp_fc.points, ImVec4(0.13f, 0.83f, 0.91f, 1.00f), 16.0f, false);
                if (frameCount - frameLastAction > 10)
                    inTarget = false;
            } else if (takeSnapshot)
                ccalib::drawRectangle(temp_fc.points, ImVec4(0.91f, 0.83f, 0.26f, 1.00f), 8.0f, true);
            else
                ccalib::drawRectangle(temp_fc.points, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), 4.0f, false);
        }

        // Draw previous recorded snapshots
        if (!snapshots.empty() && img.hasCheckerboard) {
            int n = snapshots.size() - 1;
            for (int i = n; i > 0; i--) {
                ccalib::Corners temp_fc = snapshots[i].frameCorners;
                ccalib::relativeToAbsPoints(temp_fc.points, imgSizeOld);
                ccalib::increaseRectSize(temp_fc.points, snapshots[i].frame.size * img.data.cols * 0.1f);
                if (flipImg) {
                    ccalib::flipPoints(temp_fc.points, imgSizeOld);
                }

                // Convert to img coordinates
                for (auto &p : temp_fc.points) {
                    p *= scaling;
                    p += offset;
                }
                ccalib::drawRectangle(temp_fc.points, ImVec4(0.56f, 0.83f, 0.26f, float(i) / n * 0.2f), 2.0f,
                                      false);
            }
        }

        // Display reprojection error
        if (snapshots.size() >= 4 && calibParams.reprojErr != DBL_MAX) {
            string reproj_error;
            if (snapID == -1)
                reproj_error = "Mean Reprojection Error: " + to_string(calibParams.reprojErr);
            else
                reproj_error = "Reprojection Error: " + to_string(instanceErrs[snapID]);
            float text_width = ImGui::CalcTextSize(reproj_error.c_str()).x;
            ImGui::SetCursorPos(ImVec2(pos.x + preview.cols - text_width - 16, pos.y + 17));
            ImGui::TextColored(ImColor(0, 0, 0, 255), "%s", reproj_error.c_str());
            ImGui::SetCursorPos(ImVec2(pos.x + preview.cols - text_width - 16, pos.y + 16));
            ImGui::TextColored(ImColor(255, 255, 255, 255), "%s", reproj_error.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor(1);

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        frameCount++;
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
