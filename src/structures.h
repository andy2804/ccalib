//
// Created by andya on 21.01.20.
//

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <opencv2/core/mat.hpp>
#include <imgui/imgui.h>

namespace ccalib {

    struct Corners {
        std::vector<cv::Point2f> points;
        Corners() : points{{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}} {}
        Corners(std::initializer_list<cv::Point2f> list) { for (auto &e : list) points.push_back(e); }
        cv::Point2f topLeft() { return points[0]; }
        cv::Point2f topRight() { return points[1]; }
        cv::Point2f bottomRight() { return points[2]; }
        cv::Point2f bottomLeft() { return points[3]; }
    };

    struct CheckerboardFrame {
        cv::Point2f pos = cv::Point2f(0.5f, 0.5f);
        float size = 0.5f;
        float skew = 0.5f;
    };

    struct ImageInstance {
        cv::Mat data;
        int id = 0;
        bool hasCheckerboard = false;
        ImageInstance() = default;
        ImageInstance(const cv::Size &size, const int &type) { data = cv::Mat::zeros(size, type); }
    };

    struct Snapshot {
        ImageInstance img;
        Corners frameCorners;
        CheckerboardFrame frame;
        std::vector<cv::Point2f> corners;
    };

    struct CameraParameters {
        bool autoExposure;
        int width;
        int height;
        int fps;
        float exposure;
        float ratio;
        std::string format;
    };

    struct CoverageParameters {
        float x_min = 1.0f;
        float x_max = 0.0f;
        float y_min = 1.0f;
        float y_max = 0.0f;
        float size_min = 1.0f;
        float size_max = 0.0f;
        float skew_min = 1.0f;
        float skew_max = 0.0f;
    };

    struct GUIStateVariables {
        // Switches
        bool showCamera = true;
        bool showParameters = false;
        bool showCalibration = false;
        bool showCoverage = true;
        bool showSnapshots = true;
        bool showResults = true;
        bool calibrationMode = false;
        bool changed = false;
        bool cameraOn = false;
        bool flipImg = false;
        bool undistort = false;
        bool calibrated = false;
        bool takeSnapshot = false;
        bool inTarget = false;

        // UI state variables
        ImFont *fontTitle;
        int widthParameterWindow = 350;
        float widthItemSpacing = 180;
        int frameLastAction = 0;
        int frameCount = 0;
        float imageMovement = 0.0f;
        int snapID = -1;
        float snapshotDensity = 0.06f;
        std::string loadingSequence = "/-\\|";

        // Camera specific state variables
        int camID = 0;
        int camFPS = 0;
        int camFMT = 0;
        std::vector<int> camera_fps{5, 10, 15, 20, 30, 50, 60, 100, 120};
        std::vector<std::string> camera_fmt{"YUVY", "YUY2", "YU12", "YV12", "RGB3", "BGR3", "Y16 ", "MJPG", "MPEG", "X264", "HEVC"};
        std::vector<std::string> cameras;
    };

    struct CalibrationParameters {
        cv::Mat K = cv::Mat::eye(3, 3, CV_64F);
        cv::Mat D = cv::Mat::zeros(8, 1, CV_64F);
        cv::Mat P = cv::Mat::zeros(3, 4, CV_64F);
        std::vector<cv::Mat> R, T;
        double reprojErr = DBL_MAX;
        double reprojErrVar = DBL_MAX;
    };

} // namespace ccalib

#endif // STRUCTURES_H