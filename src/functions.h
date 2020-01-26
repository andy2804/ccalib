//
// Created by andya on 23.01.20.
//

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <glad/glad.h>
#include <opencv2/core/mat.hpp>

namespace ccalib {

    void mat2Texture(cv::Mat &image, GLuint &imageTexture);

    float computeImageDiff(const cv::Mat &img1, const cv::Mat &img2, cv::Rect &rect);

    void flipPoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize, const int &direction = 0);

    void increaseRectSize(std::vector<cv::Point2f> &corners, const float &padding);

    void relativeToAbsPoint(cv::Point2f &point, const cv::Size &imgSize);

    void absToRelativePoint(cv::Point2f &point, const cv::Size &imgSize);

    void relativeToAbsPoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize);

    void absToRelativePoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize);

    void updateCoverage(const std::vector<ccalib::Snapshot> &snapshots, ccalib::CoverageParameters &coverage);

} // namespace ccalib

#endif // FUNCTIONS_H