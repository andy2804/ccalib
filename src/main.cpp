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

// As taken from opencv docs
double computeReprojectionErrors(const vector<vector<cv::Point3f> > &objectPoints,
                                 const vector<vector<cv::Point2f> > &imagePoints,
                                 const vector<cv::Mat> &rvecs, const vector<cv::Mat> &tvecs,
                                 const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                                 vector<float> &perViewErrors) {
    vector<cv::Point2f> imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for (i = 0; i < (int) objectPoints.size(); ++i) {
        projectPoints(cv::Mat(objectPoints[i]), rvecs[i], tvecs[i], cameraMatrix,  // project
                      distCoeffs, imagePoints2);
        err = norm(cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), CV_L2);              // difference

        auto n = (int) objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err * err / n);                        // save for this view
        totalErr += err * err;                                             // sum it up
        totalPoints += n;
    }

    return std::sqrt(totalErr / totalPoints);              // calculate the arithmetical mean
}

bool calibrateCamera(const int &chkbrd_rows, const int &chkbrd_cols, const float &chkbrd_size,
                     vector<ccalib::Snapshot> instances, vector<cv::Mat> &R, vector<cv::Mat> &T,
                     cv::Mat &K, cv::Mat &D, vector<float> &instance_errs, double &reprojection_err) {
    // Initialize values
    vector<cv::Point3f> corners3d;
    for (int i = 0; i < chkbrd_rows - 1; ++i)
        for (int j = 0; j < chkbrd_cols - 1; ++j)
            corners3d.emplace_back(j * chkbrd_size, i * chkbrd_size, 0);

    vector<vector<cv::Point2f>> imgPoints;
    vector<vector<cv::Point3f>> objPoints;
    for (const auto &instance : instances) {
        imgPoints.push_back(instance.corners);
        objPoints.push_back(corners3d);
    }

    const int camera_width = instances[0].img.cols;
    const int camera_height = instances[0].img.rows;

    cv::calibrateCamera(objPoints, imgPoints, cv::Size(camera_width, camera_height),
                        K, D, R, T, CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5);

    reprojection_err = computeReprojectionErrors(objPoints, imgPoints, R, T, K, D, instance_errs);
    return reprojection_err <= 0.3;
}

ImVec4 interp_color(const float &x, const float &lb, const float &ub) {
    float x_interp = (x - ub) / (lb - ub);
    return ImVec4(1.0f - x_interp, x_interp, 0.0f, 1.0f);
}

void flipPoints(vector<cv::Point2f> &points, const cv::Size &img_size, const int &direction = 0) {
    for (auto &p : points) {
        if (direction)
            p.y = img_size.height - p.y;
        else
            p.x = img_size.width - p.x;
    }
}

void increaseRectSize(vector<cv::Point2f> &corners, const float &padding) {
    vector<cv::Point2f> dir, dir2;
    for (int i = 0; i < corners.size(); i++) {
        // Push points outwards in clockwise and anti-clockwise direction
        dir.push_back(corners[i] - corners[(i + 1) % 4]);
        dir2.push_back(corners[i] - corners[(i + 3) % 4]);
    }
    for (int i = 0; i < corners.size(); i++) {
        corners[i] += (dir[i] * (padding / cv::norm(dir[i])));
        corners[i] += (dir2[i] * (padding / cv::norm(dir2[i])));
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
    ImGuiIO &io = ImGui::GetIO();
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
    int chkbrd_rows = 8;
    int chkbrd_cols = 11;
    float chkbrd_size = 0.022;      // in [m]

    // Coverage specific state variables
    float x_min = camParams.width;
    float x_max = 0;
    float y_min = camParams.height;
    float y_max = 0;
    float size_min = 1.0f;
    float size_max = 0.0f;
    float max_size = static_cast<float>(camParams.width * camParams.height);
    float skew_min = 1.0f;
    float skew_max = 0.0f;
    cv::Point2f mean(camParams.width / 2.0f, camParams.height / 2.0f);
    float size = 0.5f;
    float mid_skew = ((chkbrd_cols - 1.0f) / (chkbrd_rows - 1.0f));
//    mid_skew = mid_skew > 1 ? mid_skew - 1 : (1 / mid_skew) - 1;
    float skew = 0.5f;

    // Initialize camera matrix && dist coeff
    cv::Mat K = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat D = cv::Mat::zeros(8, 1, CV_64F);
    vector<cv::Mat> R, T;
    auto reprojection_err = DBL_MAX;
    int curr_snapshot = -1;
    vector<ccalib::Snapshot> snapshots;
    vector<float> instance_errs;
    vector<cv::Point2f> corners;
    vector<cv::Point2f> frame_corners;

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
    cv::Mat img = cv::Mat::zeros(cv::Size(camParams.width, camParams.height), CV_8UC3);
    cv::Mat img_prev = cv::Mat::zeros(cv::Size(camParams.width, camParams.height), CV_8UC3);
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
                                        cam.updateFormat(camParams.format );
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
                        ImGui::InputInt("##chkbrd_rows", &chkbrd_rows, 1);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Cols");
                        ImGui::SameLine(spacing);
                        ImGui::InputInt("##chkbrd_cols", &chkbrd_cols, 1);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Size in [m]");
                        ImGui::SameLine(spacing);
                        ImGui::InputFloat("##chkbrd_size", &chkbrd_size, 0.001f);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Calibration");
                        const char *button_text = calibration_mode ? "Reset" : "Start";
                        ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(button_text).x -
                                        style.FramePadding.x);
                        if (ccalib::MaterialButton(button_text, !calibration_mode && cam.isStreaming())) {
                            calibration_mode = !calibration_mode;
                            if (cameraOn && calibration_mode) {
                                x_min = camParams.width;
                                x_max = 0;
                                y_min = camParams.height;
                                y_max = 0;
                                mean = cv::Point2f(camParams.width / 2.0f, camParams.height / 2.0f);
                                size_min = 1.0f;
                                size_max = 0.0f;
                                max_size = camParams.width * camParams.height;
                                skew_min = 1.0f;
                                skew_max = 0.0f;
                                mid_skew = ((chkbrd_cols - 1.0f) / (chkbrd_rows - 1.0f));
                                curr_target = 0;
                                size = 0.5f;
                                skew = 0.5f;
                                reprojection_err = DBL_MAX;
                                undistort = false;
                                curr_snapshot = -1;

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
                    cam.captureFrame(img);
                    cv::cvtColor(img, img, CV_BGR2RGB);
                }

                // ==========================================
                // Calibration Snapshots
                // ==========================================

                if (calibration_mode) {
                    if (ImGui::BeginTabItem("Calibration")) {

                        // Detect Checkerboard
                        if (cam.isStreaming()) {
                            cv::Mat gray(img.rows, img.cols, CV_8UC1);
                            cv::cvtColor(img, gray, cv::COLOR_RGB2GRAY);
                            cv::cvtColor(gray, img, cv::COLOR_GRAY2RGB);
                            //                    cv::resize(gray, gray, cv::Size(gray.cols / 2, gray.rows / 2));
                            if (findCorners(gray, corners, chkbrd_cols, chkbrd_rows)) {
//                                cv::drawChessboardCorners(img, cv::Size(chkbrd_cols - 1, chkbrd_rows - 1),
//                                                          cv::Mat(corners),
//                                                          true);
                            }

                            if (corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1))) {
//                                cv::RotatedRect rect = cv::minAreaRect(corners);
//                                mean = rect.center;
//                                size = sqrt(rect.size.area() / max_size);
                                cv::Point2f ul = corners[0];
                                cv::Point2f ur = corners[chkbrd_cols - 2];
                                cv::Point2f br = corners[corners.size() - 1];
                                cv::Point2f bl = corners[corners.size() - chkbrd_cols + 1];
                                frame_corners = {ul, ur, br, bl};
                                double width = max(cv::norm(ur - ul), cv::norm(br - bl));
                                double height = max(cv::norm(bl - ul), cv::norm(br - ur));
                                mean = ul + (br - ul) / 2;
                                size = (float) sqrt(width * height / max_size);
                                skew = (float) log(width / height / mid_skew) / 3.0f + 0.5f;
                            }
                        }

                        // Show Coverage Card
                        if (ccalib::BeginCard("Coverage", font_title, 9.5,
                                      show_coverage_card)) {
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Horizontal Coverage");

                            ccalib::CoveredBar((x_min - (0.1f * camParams.width)) / camParams.width,
                                       (x_max + (0.1f * camParams.width)) / camParams.width,
                                       (camParams.width - mean.x) / camParams.width);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Vertical Coverage");

                            ccalib::CoveredBar((y_min - (0.1f * camParams.height)) / camParams.height,
                                       (y_max + (0.1f * camParams.height)) / camParams.height,
                                       (camParams.height - mean.y) / camParams.height);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Size Coverage");
                            ccalib::CoveredBar((size_min - 0.1f), (size_max + 0.1f), size);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Skew Coverage");
                            ccalib::CoveredBar((skew_min - 0.1f), (skew_max + 0.1f), skew);

                            ccalib::EndCard();
                        }

                        // Snapshots Card
                        if (ccalib::BeginCard("Snapshots", font_title, 3.3f + snapshots.size() * 0.76f, show_snapshots_card)) {
                            // Collect snapshot button
                            // TODO automatic collection of snapshots
                            const char *status_text;
                            if (ccalib::MaterialButton("Snapshot", !calibrated && snapshots.size() < 4) && cam.isStreaming())
                                taking_snapshot = true;

                            if (taking_snapshot)
                                status_text = "Try not to move...";
                            else if (!corners.empty())
                                status_text = "Ready";
                            else
                                status_text = "No Checkerboard detected!";
                            float movement_ind = 0.0f;

                            if (cam.isStreaming() && corners.size() == ((chkbrd_cols - 1) * (chkbrd_rows - 1))) {
                                // Compare actual frame with previous frame for movement
                                cv::Rect rect = cv::minAreaRect(corners).boundingRect();
                                cv::Mat old_img;
                                cv::Mat new_img;
                                if (rect.x > 0 && rect.y > 0 && rect.x + rect.width < camParams.width &&
                                    rect.y + rect.height < camParams.height) {
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
                                movement_ind = 1.0f - (float) mean_diff.val[0] / 127.0f;

                                // If successful, add instance
                                if (taking_snapshot && mean_diff.val[0] < 4) {
                                    ccalib::Snapshot instance;
                                    gettimeofday(&instance.id, nullptr);
                                    img.copyTo(instance.img);
                                    instance.corners.assign(corners.begin(), corners.end());
                                    snapshots.push_back(instance);

                                    x_min = min(x_min, camParams.width - mean.x);
                                    x_max = max(x_max, camParams.width - mean.x);
                                    y_min = min(y_min, camParams.height - mean.y);
                                    y_max = max(y_max, camParams.height - mean.y);
                                    size_min = min(size_min, size);
                                    size_max = max(size_max, size);
                                    skew_min = min(skew_min, skew);
                                    skew_max = max(skew_max, skew);

                                    if (snapshots.size() >= 4) {
                                        calibrated = calibrateCamera(chkbrd_rows, chkbrd_cols, chkbrd_size, snapshots,
                                                                     R, T, K,
                                                                     D, instance_errs, reprojection_err);
                                        undistort = true;
                                    }

                                    taking_snapshot = false;
                                    if (in_target)
                                        curr_target++;
                                }
                            }

                            ImGui::SameLine();
                            ImGui::Text("%s", status_text);
                            ccalib::CoveredBar(0.0f, movement_ind);

                            // List all snapshots
                            // TODO progress bar of how many snapshots need to be taken
                            if (!snapshots.empty()) {
                                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - 16);
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 11));
                                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.2f));
                                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
                                ImGui::BeginColumns("##snapshots", 1, ImGuiColumnsFlags_NoBorder);
                                ImGui::BeginGroup();
                                ImDrawList *drawList = ImGui::GetWindowDrawList();
                                for (int i = 0; i < snapshots.size(); i++) {
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                                    bool is_selected = i == curr_snapshot;
                                    string text;
                                    double stamp = snapshots[i].id.tv_sec + (snapshots[i].id.tv_usec / 1e6);
                                    text = to_string(i) + ": " + to_string(stamp);
                                    ImVec2 p = ImGui::GetCursorScreenPos();
                                    ImVec2 s(ImGui::GetContentRegionAvailWidth(),
                                             ImGui::GetTextLineHeight() + 4);

                                    if (instance_errs.size() > i) {
                                        ImVec4 color = interp_color(instance_errs[i], 0.0f, 1.0f);
                                        color.x *= 0.56f;
                                        color.y *= 0.83f;
                                        color.w *= 0.5f;
                                        drawList->AddRectFilled(ImVec2(p.x - 4, p.y - 4),
                                                                ImVec2(p.x + s.x + 4, p.y + s.y),
                                                                ImGui::GetColorU32(color), 3.0f);
                                    }

                                    if (ImGui::Selectable(text.c_str(), is_selected)) {
                                        if (is_selected)
                                            curr_snapshot = -1;
                                        else
                                            curr_snapshot = i;
                                    }

                                    if (is_selected) {
//                                        ImGui::SetItemDefaultFocus();
                                        drawList->AddRect(ImVec2(p.x - 5, p.y - 5),
                                                          ImVec2(p.x + s.x + 5, p.y + s.y + 1),
                                                          ImGui::GetColorU32(ImVec4(0.56f, 0.83f, 0.26f, 1.0f)),
                                                          3.0f, ImDrawCornerFlags_All, 3.0f);
                                        if (ImGui::BeginPopupContextItem()) {
                                            if (ImGui::Selectable("Remove")) {
                                                snapshots.erase(snapshots.begin() + curr_snapshot);
                                                curr_snapshot--;
                                                if (snapshots.size() >= 4) {
                                                    calibrated = calibrateCamera(chkbrd_rows, chkbrd_cols, chkbrd_size,
                                                                                 snapshots, R, T, K,
                                                                                 D, instance_errs, reprojection_err);
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
                        frame_corners.clear();
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
                            if (ccalib::MaterialButton("Re-Calibrate", false) || snapshots.size() != instance_errs.size()) {
                                calibrated = calibrateCamera(chkbrd_rows, chkbrd_cols, chkbrd_size, snapshots, R, T, K,
                                                             D, instance_errs, reprojection_err);
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
                                    fs << "camera_matrix" << K;
                                    fs << "distortion_model" << "plumb_bob";
                                    fs << "distortion_coefficients" << D;
                                    fs << "rectification_matrix" << cv::Mat::eye(3, 3, CV_64F);
                                    cv::Mat P;
                                    cv::hconcat(K, cv::Mat::zeros(3, 1, CV_64F), P);
                                    fs << "projection_matrix" << P;
                                    fs.release();

                                    ImGui::SameLine();
                                    ImGui::Text("Exported!");
                                }
                                stringstream result_ss;
                                result_ss << "K = " << K << endl << endl;
                                result_ss << "D = " << D;
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

                if (curr_snapshot != -1) {
                    img = snapshots[curr_snapshot].img;
                    corners = snapshots[curr_snapshot].corners;
                } else if (cam.isOpened() && !cam.isStreaming()) {
                    cam.captureFrame(img);
                    cv::cvtColor(img, img, CV_BGR2RGB);
                }
            }

            img.copyTo(img_prev);

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
                if (cam.isStreaming()) {
                    if (flip_img)
                        cv::flip(img, img, 1);
                    if (undistort) {
                        cv::Mat undistorted;
                        cv::undistort(img, undistorted, K, D);
                        undistorted.copyTo(img);
                    }
                }
                glDeleteTextures(1, &texture);
                mat2Texture(img, texture);
            }

            // Camera image
            float width_avail = ImGui::GetContentRegionAvail().x;
            float height_avail = ImGui::GetContentRegionAvail().y;
            float scaling = 1.0f;
            cv::Size img_size_old(img.cols, img.rows);
            if (!img.empty()) {
                if (height_avail * camParams.ratio > width_avail) {
                    scaling = width_avail / img.cols;
                    cv::resize(img, img, cv::Size((int) width_avail, (int) (width_avail / camParams.ratio)));
                } else {
                    scaling = height_avail / img.rows;
                    cv::resize(img, img, cv::Size((int) (height_avail * camParams.ratio), (int) height_avail));
                }
            }

            // Positioning && Centering
            ImVec2 pos = ImVec2((width_avail - img.cols) / 2 + ImGui::GetCursorPosX(),
                                (height_avail - img.rows) / 2 + ImGui::GetCursorPosY());
            ImGui::SetCursorPos(pos);
            ImGui::Image((void *) texture, ImVec2(img.cols, img.rows));
            cv::Point2f offset(pos.x + width_parameter_window, pos.y);

            // Draw Corners
            if (calibration_mode && !corners.empty()) {
                if (flip_img && curr_snapshot == -1) {
                    flipPoints(corners, img_size_old);
                }
                for (auto &p : corners) {
                    p *= scaling;
                    p += offset;
                }
                ccalib::drawPoints(corners, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), size * 4.0f);
            }

            // Draw Frame around checkerboard
            if (calibration_mode && !corners.empty() && !frame_corners.empty()) {
                increaseRectSize(frame_corners, size * 64);
                if (flip_img) {
                    flipPoints(frame_corners, img_size_old);
                }

                // Convert to img coordinates
                for (auto &p : frame_corners) {
                    p *= scaling;
                    p += offset;
                }
                if (curr_target >= target_frames.size())
                    ccalib::drawRectangle(frame_corners, ImVec4(0.56f, 0.83f, 0.26f, 1.00f), 4.0f, false);
            }

            // Draw target frames
            if (calibration_mode && curr_target < target_frames.size()) {
                // Convert and flip points
                vector<cv::Point2f> target_corners(target_frames[curr_target]);
                if (flip_img) {
                    flipPoints(target_corners, cv::Size(1, 1));
                }

                float chkbrd_ratio = (float) chkbrd_cols / (float) chkbrd_rows;
                float ratio_offset = (img.rows * camParams.ratio - img.rows * chkbrd_ratio) / 2.0f;
                for (int i = 0; i < frame_corners.size(); i++) {
                    target_corners[i].x = target_corners[i].x * img.rows * chkbrd_ratio + ratio_offset + offset.x;
                    target_corners[i].y = target_corners[i].y * img.rows + offset.y;
                }

                if (!corners.empty() && !frame_corners.empty() && curr_snapshot == -1) {
                    double dist = cv::norm((target_corners[0] + (target_corners[2] - target_corners[0]) / 2) -
                                           (frame_corners[0] + (frame_corners[2] - frame_corners[0]) / 2));
                    double frameArea = cv::contourArea(frame_corners);
                    double targetArea = cv::contourArea(target_corners);

                    ImVec4 col_bg = interp_color((float) dist, 0, img.rows / 2.0f);

                    if (dist <= size * 64 * scaling && frameArea > targetArea * 0.8f && frameArea < targetArea * 1.2f) {
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
                    col_bg = interp_color((float) area_diff, 0, 1.0f);
                    ccalib::drawRectangle(frame_corners, col_bg, 4.0f, true);
                }
            }

            if (reprojection_err != DBL_MAX) {
                string reproj_error;
                if (curr_snapshot == -1)
                    reproj_error = "Mean Reprojection Error: " + to_string(reprojection_err);
                else
                    reproj_error = "Reprojection Error: " + to_string(instance_errs[curr_snapshot]);
                float text_width = ImGui::CalcTextSize(reproj_error.c_str()).x;
                ImGui::SetCursorPos(ImVec2(pos.x + img.cols - text_width - 16, pos.y + 17));
                ImGui::TextColored(ImColor(0, 0, 0, 255), "%s", reproj_error.c_str());
                ImGui::SetCursorPos(ImVec2(pos.x + img.cols - text_width - 16, pos.y + 16));
                ImGui::TextColored(ImColor(255, 255, 255, 255), "%s", reproj_error.c_str());
            }

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

