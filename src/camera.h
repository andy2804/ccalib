//
// Created by andya on 02.07.18.
//

#ifndef V4L2_CAMERA_H
#define V4L2_CAMERA_H

#include <string>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include "structures.cpp"

namespace ccalib {

    class Camera {
    private:
        cv::VideoCapture camera;
        bool cameraOn = false;
        bool streamOn = false;

    public:
        ccalib::CameraParameters params;

        Camera(const std::string &device_address);

        ~Camera();

        void connect();

        void startStream();

        void stopStream();

        void release();

        void updateParameters();

        uint32_t fourcc(const char *p);

        void grab();

        void captureFrame(cv::Mat &destination);

        void updateResolution(const int &width, const int &height);

        void updateExposure(const float &exposure);

        void updateFramerate(const int &fps);

        void updateFormat(const std::string &format);
    };

} // namespace ccalib

#endif
