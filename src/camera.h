//
// Created by andya on 02.07.18.
//

#ifndef V4L2_CAMERA_H
#define V4L2_CAMERA_H

#include <string>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include "structures.cpp"

namespace ccalib {

    class Camera {
    private:
        cv::VideoCapture camera;

    public:
        ccalib::CameraParameters params;

        Camera(const std::string &device_address);

        ~Camera();

        void init();

        void setFramerate(int fps);

        void startStream();

        void stopStream();

        void closeConnection();

        cv::Mat captureRawFrame();

        void release();
    };

} // namespace ccalib

#endif
