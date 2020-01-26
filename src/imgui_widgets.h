//
// Created by andya on 23.01.20.
//

#ifndef IMGUI_WIDGETS_H
#define IMGUI_WIDGETS_H

#include "structures.h"
#include "camera.h"

namespace ccalib {

    void CameraCard (ccalib::GUIStateVariables &state, ccalib::Camera &cam, ccalib::CameraParameters &camParams);

    void CameraParametersCard(ccalib::GUIStateVariables &state, ccalib::Camera &cam,
                              ccalib::CameraParameters &camParams);

} // namespace ccalib

#endif // FUNCTIONS_H