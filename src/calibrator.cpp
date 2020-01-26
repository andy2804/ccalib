//
// Created by andya on 24.01.20.
//

#include "calibrator.h"
#include "functions.h"

#include <thread>
#include <numeric>


using namespace std;


namespace ccalib {

    Calibrator::Calibrator() {}

    Calibrator::Calibrator(int rows, int cols, float size) : checkerboardRows(rows), checkerboardCols(cols),
                                                             checkerboardSize(size) {}

    Calibrator::~Calibrator() {}

    bool Calibrator::isCalibrating() {
        return busy;
    }

    bool Calibrator::findCorners(cv::Mat &img, std::vector<cv::Point2f> &corners) {
        if (img.channels() == 3)
            cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);
        if (cv::findChessboardCorners(img, cv::Size(checkerboardCols - 1, checkerboardRows - 1), corners,
                                      CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE |
                                      CV_CALIB_CB_FAST_CHECK))
            cv::cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
        cv::cvtColor(img, img, cv::COLOR_GRAY2RGB);
        return corners.size() == (checkerboardRows - 1) * (checkerboardCols - 1);
    }

    void Calibrator::computeFrame(const std::vector<cv::Point2f> &corners, const ccalib::CameraParameters &camParams,
                                  ccalib::CheckerboardFrame &frame, ccalib::Corners &frameCorners) {
        ccalib::Corners fc({corners[0], corners[checkerboardCols - 2], corners[corners.size() - 1],
                            corners[corners.size() - checkerboardCols + 1]});
        ccalib::absToRelativePoints(fc.points, cv::Size(camParams.width, camParams.height));
        double width = max(cv::norm(fc.topRight() - fc.topLeft()), cv::norm(fc.bottomRight() - fc.bottomLeft()));
        double height = max(cv::norm(fc.bottomLeft() - fc.topLeft()), cv::norm(fc.bottomRight() - fc.topRight()));
        float skewRatio = ((checkerboardCols - 1.0f) / (checkerboardRows - 1.0f));
        frame.pos = fc.topLeft() + (fc.bottomRight() - fc.topLeft()) / 2;
        frame.size = (float) sqrt(width * height);
        frame.skew = (float) log(width / height / skewRatio * camParams.ratio) / 3.0f + 0.5f;
        frameCorners = fc;
    }

    // As taken from opencv docs
    double Calibrator::computeReprojectionErrors(const std::vector<std::vector<cv::Point3f> > &objectPoints,
                                                 const std::vector<std::vector<cv::Point2f> > &imagePoints,
                                                 const ccalib::CalibrationParameters &params,
                                                 std::vector<double> &perViewErrors) {
        std::vector<cv::Point2f> imagePoints2;
        int i, totalPoints = 0;
        double totalErr = 0, err;
        perViewErrors.resize(objectPoints.size());

        for (i = 0; i < (int) objectPoints.size(); ++i) {
            projectPoints(cv::Mat(objectPoints[i]), params.R[i], params.T[i], params.K, params.D, imagePoints2);
            err = norm(cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), CV_L2); // difference

            auto n = (int) objectPoints[i].size();
            perViewErrors[i] = std::sqrt(err * err / n); // save for this view
            totalErr += err * err; // sum it up
            totalPoints += n;
        }

        return std::sqrt(totalErr / totalPoints); // calculate the arithmetical mean
    }

    void Calibrator::calibrateCameraBG(const std::vector<ccalib::Snapshot> &instances, ccalib::CalibrationParameters &params,
                                  std::vector<double> &errs) {
        std::thread t(&Calibrator::calibrateCamera, this, std::ref(instances), std::ref(params), std::ref(errs));
        t.detach();
    }

    bool Calibrator::calibrateCamera(const std::vector<ccalib::Snapshot> &instances, ccalib::CalibrationParameters &params,
                                std::vector<double> &errs) {
        // Initialize values
        busy = true;
        std::vector<cv::Point3f> corners3d;
        std::vector<ccalib::Snapshot> instancesCopy(instances);
        for (int i = 0; i < checkerboardRows - 1; ++i)
            for (int j = 0; j < checkerboardCols - 1; ++j)
                corners3d.emplace_back(j * checkerboardSize, i * checkerboardSize, 0);

        std::vector<std::vector<cv::Point2f>> imgPoints;
        std::vector<std::vector<cv::Point3f>> objPoints;
        for (const auto &instance : instancesCopy) {
            imgPoints.push_back(instance.corners);
            objPoints.push_back(corners3d);
        }

        const int camera_width = instancesCopy[0].img.data.cols;
        const int camera_height = instancesCopy[0].img.data.rows;

        cv::calibrateCamera(objPoints, imgPoints, cv::Size(camera_width, camera_height),
                            params.K, params.D, params.R, params.T,
                            CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5);

        params.reprojErr = computeReprojectionErrors(objPoints, imgPoints, params, errs);
        params.reprojErrVar = stddev(errs);
        busy = false;
        return params.reprojErr <= 0.3f && params.reprojErrVar <= 0.1f;
    }

    double Calibrator::stddev(std::vector<double> const &func) {
        double mean = std::accumulate(func.begin(), func.end(), 0.0) / func.size();
        double sq_sum = std::inner_product(func.begin(), func.end(), func.begin(), 0.0,
                                           [](double const &x, double const &y) { return x + y; },
                                           [mean](double const &x, double const &y) {
                                               return (x - mean) * (y - mean);
                                           });
        return std::sqrt(sq_sum / (func.size() - 1));
    }

} // namespace ccalib