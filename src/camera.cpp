//
// Created by andya on 02.07.18.
//

#include "camera.h"

#include <iostream>
#include <string>
#include <stdio.h>
#include "structures.cpp"

#include <opencv2/opencv.hpp>


using namespace std;


/**
 * =====================================================================
 * V4L2 Device Manager Class using OpenCV VideoCapture
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
        params.autoExposure = true;
        params.exposure = 0.333;
    }

    Camera::~Camera() {
        stopStream();
        closeConnection();
    }

    void Camera::init() {
        // Open camera connection
        camera.open(device);
        if (!camera.isOpened())
            printf("Camera %s could not be opened!", device);
    }

    void Camera::updateParameters() {

    }

    void Camera::setFramerate(int fps) {
        // Set framerate struct
    }

    cv::Mat Camera::captureRawFrame() {
    }

    void Camera::release() {
        camera.release();
    }

    void Camera::startStream() {
        // Activate streaming
    }

    void Camera::stopStream() {
        // Deactivate streaming
    }

} // namespace ccalib