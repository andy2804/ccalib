//
// Created by andya on 02.07.18.
//

#include "camera.h"

#include <iostream>
#include <string>
#include <stdio.h>

#include <opencv2/opencv.hpp>
#include <thread>


using namespace std;


/**
 * =====================================================================
 * V4L2 Device Manager Class using OpenCV VideoCapture
 * =====================================================================
 * Let's the user create a new device and continously grab frames from
 * the specified camera in the background.
 * =====================================================================
 */

namespace ccalib {

    Camera::Camera(const string &device_address) {
        // Initialize Camera
        params.device = device_address;

        // Use default values
        params.width = 640;
        params.height = 480;
        params.framerate = 30;
        params.format = "YUVY";
        params.autoExposure = false;
        params.exposure = 0.333;
    }

    Camera::~Camera() {
        stopStream();
        release();
    }

    void Camera::connect() {
        // Open camera connection
        camera.open(params.device);
        if (!camera.isOpened())
            printf("Camera %s could not be opened!", params.device.c_str());
        cameraOn = true;

        // Update parameters
        updateParameters();
    }

    void Camera::updateParameters() {
        camera.set(CV_CAP_PROP_FOURCC, fourcc(params.format.c_str()));
        camera.set(CV_CAP_PROP_FPS, (double) params.framerate);
        camera.set(CV_CAP_PROP_FRAME_WIDTH, (double) params.width);
        camera.set(CV_CAP_PROP_FRAME_HEIGHT, (double) params.height);
        camera.set(CV_CAP_PROP_AUTO_EXPOSURE, params.autoExposure ? 0.75 : 0.25);
        camera.set(CV_CAP_PROP_EXPOSURE, params.exposure);
    }

    void Camera::updateFramerate(const int &fps) {
        params.framerate = fps;
        camera.set(CV_CAP_PROP_FPS, (double) params.framerate);
    }

    void Camera::updateExposure(const float &exposure) {
        params.exposure = exposure;
        camera.set(CV_CAP_PROP_EXPOSURE, params.exposure);
    }

    void Camera::updateFormat(const string &format) {
        params.format = format;
        camera.set(CV_CAP_PROP_FOURCC, fourcc(params.format.c_str()));
    }

    void Camera::updateResolution(const int &width, const int &height) {
        params.width = width;
        params.height = height;
        camera.set(CV_CAP_PROP_FRAME_WIDTH, (double) params.width);
        camera.set(CV_CAP_PROP_FRAME_HEIGHT, (double) params.height);
    }

    void Camera::release() {
        streamOn = false;
        cameraOn = false;
        camera.release();
    }

    void Camera::grab() {
        while (streamOn) {
            camera.grab();
        }
    }

    void Camera::captureFrame(cv::Mat &destination) {
        // Retrieve latest frame if camera is streaming, else return black frame
        if (streamOn)
            camera.retrieve(destination);
//        else
//            destination = cv::Mat::zeros(params.height, params.width, CV_8UC3);
    }

    void Camera::startStream() {
        // Activate streaming
        if (cameraOn) {
            streamOn = true;
            std::thread t(&Camera::grab, this);
            t.detach();
        }
    }

    void Camera::stopStream() {
        // Deactivate streaming
        streamOn = false;
    }

    uint32_t Camera::fourcc(char const p[5]) {
        return (((p[0]) & 255) + (((p[1]) & 255) << 8) + (((p[2]) & 255) << 16) + (((p[3]) & 255) << 24));
    }

} // namespace ccalib