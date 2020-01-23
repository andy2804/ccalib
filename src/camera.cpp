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

    Camera::Camera() {
        Camera(device);
    }

    Camera::Camera(const string &device_address) {
        // Initialize Camera
        device = device_address;

        // Use default values
        params.width = 640;
        params.height = 480;
        params.ratio = 640.0f / 480.0f;
        params.framerate = 30;
        params.format = "YUVY";
        params.autoExposure = false;
        params.exposure = 0.333;
    }

    Camera::Camera(const string &device_address, const ccalib::CameraParameters &camParams) {
        device = device_address;
        params = camParams;
    }

    Camera::~Camera() {
        stopStream();
        close();
    }

    void Camera::open() {
        // Open camera connection
        cout << "Opening: " << device << endl;
        camera.open(device);
        if (!isOpened())
            printf("Camera %s could not be opened!", device.c_str());
        else
            cout << "Connection established!" << endl;

        // Update parameters
        updateParameters();
    }

    void Camera::open(const string &device_address) {
        device = device_address;
        open();
    }

    bool Camera::isOpened() {
        return camera.isOpened();
    }

    bool Camera::isStreaming() {
        return streamOn;
    }

    void Camera::updateParameters() {
        if (isOpened() && !streamOn) {
            // Set new params
            camera.set(CV_CAP_PROP_FOURCC, fourcc(params.format.c_str()));
            camera.set(CV_CAP_PROP_FPS, params.framerate);
            camera.set(CV_CAP_PROP_FRAME_WIDTH, params.width);
            camera.set(CV_CAP_PROP_FRAME_HEIGHT, params.height);
            camera.set(CV_CAP_PROP_AUTO_EXPOSURE, params.autoExposure ? 0.75 : 0.25);
            camera.set(CV_CAP_PROP_EXPOSURE, params.exposure);

            // Update with actual settings
            uint32_t fourcc = camera.get(CV_CAP_PROP_FOURCC);
            params.format = cv::format("%c%c%c%c", fourcc & 255, (fourcc >> 8) & 255, (fourcc >> 16) & 255, (fourcc >> 24) & 255);
            params.framerate = camera.get(CV_CAP_PROP_FPS);
            params.width = camera.get(CV_CAP_PROP_FRAME_WIDTH);
            params.height = camera.get(CV_CAP_PROP_FRAME_HEIGHT);
            params.exposure = camera.get(CV_CAP_PROP_EXPOSURE);
        } else if (isOpened()) {
            stopStream();
            updateParameters();
            startStream();
        }
    }

    ccalib::CameraParameters Camera::getParameters() {
        return params;
    }

    void Camera::updateParameters(ccalib::CameraParameters &newParams) {
        params = newParams;
        updateParameters();
    }

    void Camera::updateFramerate(const double &fps) {
        params.framerate = fps;
        updateParameters();
    }

    void Camera::updateExposure(const float &exposure) {
        params.exposure = (double) exposure;
        camera.set(CV_CAP_PROP_EXPOSURE, params.exposure);
    }

    void Camera::updateFormat(const string &format) {
        params.format = format;
        updateParameters();
    }

    void Camera::updateResolution(const double &width, const double &height) {
        params.width = (double) width;
        params.height = (double) height;
        params.ratio = (double) width / (double) height;
        updateParameters();
    }

    double Camera::getRatio() {
        return params.ratio;
    }

    void Camera::close() {
        cout << "Closing camera... ";
        while (streamOn) {
            streamFlag = false;
        }
        while (isOpened())
            camera.release();
        cout << "closed." << endl;
    }

    void Camera::grab() {
        cout << "Streaming..." << endl;
        while (streamFlag) {
            cout << "." << endl;
            if (camera.grab())
                camera.retrieve(image);
        }
        streamOn = false;
        cout << "Stopped streaming." << endl;
    }

    void Camera::captureFrame(cv::Mat &destination) {
        // Retrieve latest frame if camera is streaming, else return black frame
        if (streamOn && !image.empty())
            image.copyTo(destination);
        else
            destination = cv::Mat::zeros(params.height, params.width, CV_8UC3);
    }

    void Camera::startStream() {
        // Activate streaming
        cout << "Start streaming... ";
        if (isOpened()) {
            streamOn = true;
            streamFlag = true;
            std::thread t(&Camera::grab, this);
            t.detach();
        }
        cout << "started." << endl;
    }

    void Camera::stopStream() {
        // Deactivate streaming
        while (streamOn) {
            streamFlag = false;
        }
    }

    constexpr uint32_t Camera::fourcc(char const p[5]) {
        return (((p[0]) & 255) + (((p[1]) & 255) << 8) + (((p[2]) & 255) << 16) + (((p[3]) & 255) << 24));
    }

} // namespace ccalib