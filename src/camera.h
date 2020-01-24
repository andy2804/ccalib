//
// Created by andya on 02.07.18.
//

#ifndef CAMERA_H
#define CAMERA_H

#include <string>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include "structures.h"

namespace ccalib {

    class Camera {
    private:
        bool streamOn = false;
        bool streamFlag = false;
        int frameCount = 0;

        std::string device = "/dev/video0";
        cv::VideoCapture camera;
        cv::Mat image;

        ccalib::CameraParameters params;

    public:

        Camera();

        Camera(const std::string &device_address, const CameraParameters &camParams);

        Camera(const std::string &device_address);

        ~Camera();

        void open();

        void startStream();

        void stopStream();

        void close();

        void updateParameters();

        constexpr uint32_t fourcc(const char *p);

        void grab();

        int captureFrame(cv::Mat &destination);

        void updateResolution(const int &width, const int &height);

        void updateExposure(const float &exposure);

        void updateFramerate(const int &fps);

        void updateFormat(const std::string &format);

        void updateParameters(CameraParameters &newParams);

        bool isOpened();

        bool isStreaming();

        CameraParameters getParameters();

        double getRatio();

        void open(const std::string &device_address);
    };

} // namespace ccalib

#endif // CAMERA_H
