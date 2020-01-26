//
// Created by andya on 21.01.20.
//

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <opencv2/core/mat.hpp>

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
        bool show_demo_window = false;
        bool show_camera_card = true;
        bool show_parameters_card = false;
        bool show_calibration_card = true;
        bool show_coverage_card = false;
        bool show_snapshots_card = true;
        bool show_result_card = true;
        bool calibration_mode = false;
        bool cameraOn = false;
        bool changed = false;
        bool flip_img = false;
        bool undistort = false;
        bool calibrated = false;
        bool taking_snapshot = false;
        bool in_target = false;

        // Current list values
        int currCamera = 0;
        int currTarget = 0;

        // Calibration state Variables
        int chkbrd_rows = 8;
        int chkbrd_cols = 11;
        float chkbrd_size = 0.022;      // in [m]

        // UI state variables
        int width_parameter_window = 350;
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