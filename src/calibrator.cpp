//
// Created by andya on 24.01.20.
//

#include "calibrator.h"

#include <iostream>
#include <string>
#include <stdio.h>

#include <opencv2/opencv.hpp>
#include <thread>


using namespace std;


namespace ccalib {

    Calibrator::Calibrator() {}

    Calibrator::Calibrator(int rows, int cols, float size) : checkerboardRows(rows), checkerboardCols(cols),
                                                             checkerboardSize(size) {}

    Calibrator::~Calibrator() {}

    bool Calibrator::findCorners(cv::Mat &img, std::vector<cv::Point2f> &corners) {
        bool found = false;
        if (img.channels() == 3)
            cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
        if (cv::findChessboardCorners(img, cv::Size(checkerboardCols - 1, checkerboardRows - 1), corners,
                                      CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE |
                                      CV_CALIB_CB_FAST_CHECK)) {
            cv::cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
            found = true;
        }
        cv::cvtColor(img, img, cv::COLOR_GRAY2RGB);
        return found;
    }

    // As taken from opencv docs
    double Calibrator::computeReprojectionErrors(const std::vector<std::vector<cv::Point3f> > &objectPoints,
                                                 const std::vector<std::vector<cv::Point2f> > &imagePoints,
                                                 const ccalib::CalibrationParameters &params,
                                                 std::vector<float> &perViewErrors) {
        std::vector<cv::Point2f> imagePoints2;
        int i, totalPoints = 0;
        double totalErr = 0, err;
        perViewErrors.resize(objectPoints.size());

        for (i = 0; i < (int) objectPoints.size(); ++i) {
            projectPoints(cv::Mat(objectPoints[i]), params.R[i], params.T[i], params.K, params.D, imagePoints2);
            err = norm(cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), CV_L2); // difference

            auto n = (int) objectPoints[i].size();
            perViewErrors[i] = (float) std::sqrt(err * err / n); // save for this view
            totalErr += err * err; // sum it up
            totalPoints += n;
        }

        return std::sqrt(totalErr / totalPoints); // calculate the arithmetical mean
    }

//    void Calibrator::calibrateCameraBG(std::vector<ccalib::Snapshot> instances, ccalib::CalibrationParameters &params,
//                                       std::vector<float> &errs) {
//        std::thread t(&Calibrator::calibrateCamera, this, instances, params, errs);
//        t.detach();
//    }

    bool Calibrator::calibrateCamera(std::vector<ccalib::Snapshot> instances, ccalib::CalibrationParameters &params,
                                     std::vector<float> &errs) {
        // Initialize values
        std::vector<cv::Point3f> corners3d;
        for (int i = 0; i < checkerboardRows - 1; ++i)
            for (int j = 0; j < checkerboardCols - 1; ++j)
                corners3d.emplace_back(j * checkerboardSize, i * checkerboardSize, 0);

        std::vector<std::vector<cv::Point2f>> imgPoints;
        std::vector<std::vector<cv::Point3f>> objPoints;
        for (const auto &instance : instances) {
            imgPoints.push_back(instance.corners);
            objPoints.push_back(corners3d);
        }

        const int camera_width = instances[0].img.data.cols;
        const int camera_height = instances[0].img.data.rows;

        cv::calibrateCamera(objPoints, imgPoints, cv::Size(camera_width, camera_height),
                            params.K, params.D, params.R, params.T,
                            CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5);

        params.reprojErr = computeReprojectionErrors(objPoints, imgPoints, params, errs);
        return params.reprojErr <= 0.3f;
    }

} // namespace ccalib