//
// Created by andya on 23.01.20.
//

#ifndef IMGUI_EXTENSIONS_H
#define IMGUI_EXTENSIONS_H

#include "opencv2/opencv.hpp"

namespace ccalib {

    void StyleColorsMaterial();

    void ToggleButton(const char *str_id, bool *v, const bool focus = false);

    void CoveredBar(const float &start, const float &stop, const float &indicator = -1);

    bool MaterialButton(const char *label, bool focus = false, const ImVec2 &size = ImVec2(0, 0));

    bool BeginCard(const char *label, ImFont *title_font, const float &item_height, bool &visible);

    void EndCard();

    void drawRectangle(const std::vector<cv::Point2f> &corners, const ImVec4 &color, const float &thickness = 1.0f,
                       bool focus = false);

    void drawPoints(const std::vector<cv::Point2f> &corners, const ImVec4 &color, const float &size = 1.0f,
                    bool focus = false);

} // namespace ccalib

#endif // FUNCTIONS_H