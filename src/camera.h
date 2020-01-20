//
// Created by andya on 02.07.18.
//

#ifndef V4L2_CAMERA_H
#define V4L2_CAMERA_H

#include <string>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>

class V4L2Camera {
    int fd;
    std::string device;
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buffer;
    cv::Mat raw_input;

public:
    int width;
    int height;
    timeval last_ts;

    V4L2Camera();
    V4L2Camera(std::string device_address, int width, int height);
    ~V4L2Camera();
    void init();
    void setFramerate(int fps);
    void allocateBuffer();
    void startStream();
    void stopStream();
    void closeConnection();
    cv::Mat captureRawFrame();
};

#endif
