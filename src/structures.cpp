//
// Created by andya on 21.01.20.
//

namespace ccalib {

    struct Snapshot {
        timeval id;
        cv::Mat img;
        vector <cv::Point2f> corners;
        float x, y, size, skew;
    };

    struct CameraParameters {
        int width;
        int height;
        float exposure;
        bool autoExposure;
        int framerate;
        string format;
        string device;
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
        bool camera_on = false;
        bool stream_on = false;
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
        cv::Mat P = cv::Mat::zeros(4, 3, CV_64F);
        vector<cv::Mat> R, T;
        double reprojection_err = DBL_MAX;
    };

} // namespace ccalib