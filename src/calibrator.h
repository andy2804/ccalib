//
// Created by andya on 24.01.20.
//

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <string>
#include <opencv2/opencv.hpp>
#include "structures.h"

namespace ccalib {

    class Calibrator {

    public:

        int checkerboardRows;
        int checkerboardCols;
        float checkerboardSize; // in [m]

        Calibrator();

        Calibrator(int rows, int cols, float size);

        ~Calibrator();

        bool findCorners(cv::Mat &img, std::vector<cv::Point2f> &corners);

        double computeReprojectionErrors(const std::vector<std::vector<cv::Point3f>> &objectPoints,
                                     const std::vector<std::vector<cv::Point2f>> &imagePoints,
                                     const CalibrationParameters &params, std::vector<double> &perViewErrors);

        bool calibrateCamera(const std::vector<Snapshot> &instances, CalibrationParameters &params, std::vector<double> &errs);

        void calibrateCameraBG(const std::vector<Snapshot> &instances, CalibrationParameters &params, std::vector<double> &errs);

        double stddev(std::vector<double> const & func);

    };

} // namespace ccalib

#endif // CALIBRATION_H
