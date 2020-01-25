//
// Created by andya on 23.01.20.
//

#include "structures.h"
#include "functions.h"

namespace ccalib {

    void mat2Texture(cv::Mat &image, GLuint &imageTexture) {
        if (!image.empty()) {
            //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            glGenTextures(1, &imageTexture);
            glBindTexture(GL_TEXTURE_2D, imageTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Set texture clamping method
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

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

    void flipPoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize, const int &direction) {
        for (auto &p : points) {
            if (direction)
                p.y = imgSize.height - p.y;
            else
                p.x = imgSize.width - p.x;
        }
    }

    void increaseRectSize(std::vector<cv::Point2f> &corners, const float &padding) {
        std::vector<cv::Point2f> dir, dir2;
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

    void relativeToAbsPoint(cv::Point2f &point, const cv::Size &imgSize) {
        point.x *= imgSize.width;
        point.y *= imgSize.height;
    }

    void absToRelativePoint(cv::Point2f &point, const cv::Size &imgSize) {
        point.x /= imgSize.width;
        point.y /= imgSize.height;
    }

    void relativeToAbsPoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize) {
        for (auto &p : points)
            relativeToAbsPoint(p, imgSize);
    }

    void absToRelativePoints(std::vector<cv::Point2f> &points, const cv::Size &imgSize) {
        for (auto &p : points)
            absToRelativePoint(p, imgSize);
    }

    void updateCoverage(const std::vector<ccalib::Snapshot> &snapshots, ccalib::CoverageParameters &coverage) {
        // Get default values for coverage and safe current position
        ccalib::CoverageParameters newCoverage;

        // Loop through all snapshots
        for (const auto &s : snapshots) {
            newCoverage.x_min = std::min(newCoverage.x_min, 1.0f - s.frame.pos.x);
            newCoverage.x_max = std::max(newCoverage.x_max, 1.0f - s.frame.pos.x);
            newCoverage.y_min = std::min(newCoverage.y_min, 1.0f - s.frame.pos.y);
            newCoverage.y_max = std::max(newCoverage.y_max, 1.0f - s.frame.pos.y);
            newCoverage.size_min = std::min(newCoverage.size_min, s.frame.size);
            newCoverage.size_max = std::max(newCoverage.size_max, s.frame.size);
            newCoverage.skew_min = std::min(newCoverage.skew_min, s.frame.skew);
            newCoverage.skew_max = std::max(newCoverage.skew_max, s.frame.skew);
        }

        coverage = newCoverage;
    }

} // namespace ccalib