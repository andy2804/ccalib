//
// Created by andya on 23.01.20.
//

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_widgets.h"
#include "imgui_extensions.h"
#include "calibrator.h"

#include <vector>

namespace ccalib {

    void CameraCard(ccalib::GUIStateVariables &state, ccalib::Camera &cam, ccalib::CameraParameters &camParams) {
        if (ccalib::BeginCard("Camera", state.fontTitle, 4.5f + state.calibrated, state.showCamera)) {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Device");
            ImGui::SameLine(state.widthItemSpacing);

            if (ImGui::BeginCombo("##camera_selector", state.cameras[state.camID].c_str(), 0)) {
                for (int i = 0; i < state.cameras.size(); i++) {
                    bool is_selected = (state.cameras[state.camID] == state.cameras[i]);
                    if (ImGui::Selectable(state.cameras[i].c_str(), is_selected)) {
                        if (state.cameraOn) {
                            cam.stopStream();
                            cam.close();
                            state.cameraOn = false;
                        }
                        state.camID = i;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Stream");
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
            ccalib::ToggleButton("##cam_toggle", &state.cameraOn, !state.cameraOn);
            if (state.cameraOn && ImGui::IsItemClicked(0)) {
                cam.open();
                cam.updateParameters(camParams);
                cam.startStream();
                state.camParamsChanged = true;
            } else if (!state.cameraOn & ImGui::IsItemClicked(0)) {
                cam.stopStream();
                cam.close();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Flip Image");
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
            ccalib::ToggleButton("##flip_toggle", &state.flipImg);

            if (state.calibrated) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Undistort Image");
                ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.8f);
                ccalib::ToggleButton("##undistort_toggle", &state.undistort);
            }
            ccalib::EndCard();
        }
    }

    void CameraParametersCard(ccalib::GUIStateVariables &state, ccalib::Camera &cam,
                              ccalib::CameraParameters &camParams) {
        if (ccalib::BeginCard("Parameters", state.fontTitle, 5.5, state.showParameters)) {
            ImGuiStyle &style = ImGui::GetStyle();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Resolution");
            ImGui::SameLine(state.widthItemSpacing);
            ImGui::PushItemWidth(44);
            ImGui::InputInt("##width", &camParams.width, 0);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("x");
            ImGui::SameLine();
            ImGui::InputInt("##height", &camParams.height, 0);
            ImGui::PopItemWidth();
            const char *button_text = "Set";
            ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(button_text).x -
                            style.FramePadding.x);
            if (ccalib::MaterialButton(button_text)) {
                cam.updateResolution(camParams.width, camParams.height);
                state.camParamsChanged = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Framerate");
            ImGui::SameLine(state.widthItemSpacing);
            if (ImGui::BeginCombo("##cameraFPS", std::to_string(state.cameraFPS[state.fpsID]).c_str(), 0)) {
                for (int i = 0; i < state.cameraFPS.size(); i++) {
                    bool is_selected = (state.cameraFPS[state.fpsID] == state.cameraFPS[i]);
                    if (ImGui::Selectable(std::to_string(state.cameraFPS[i]).c_str(), is_selected)) {
                        state.fpsID = i;
                        if (!ImGui::IsMouseClicked(0)) {
                            camParams.fps = (int) state.cameraFPS[state.fpsID];
                            cam.updateFramerate(camParams.fps);
                        }
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
                state.camParamsChanged = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Exposure Time");
            ImGui::SameLine(state.widthItemSpacing);
            if (ImGui::SliderFloat("##camera_exptime", &camParams.exposure, 0, 1, "%.3f", 2.0)) {
                cam.updateExposure(camParams.exposure);
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Select Format");
            ImGui::SameLine(state.widthItemSpacing);
            if (ImGui::BeginCombo("##cameraFMTS", state.cameraFMTS[state.fmtID].c_str(), 0)) {
                for (int i = 0; i < state.cameraFMTS.size(); i++) {
                    bool is_selected = (state.cameraFMTS[state.fmtID] == state.cameraFMTS[i]);
                    if (ImGui::Selectable(state.cameraFMTS[i].c_str(), is_selected)) {
                        state.fmtID = i;
                        if (!ImGui::IsMouseClicked(0)) {
                            camParams.format = state.cameraFMTS[state.fmtID];
                            cam.updateFormat(camParams.format);
                        }
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
                ImGui::EndCombo();
                state.camParamsChanged = true;
            }
            ccalib::EndCard();
        }
    }

    void CalibrationParametersCard(ccalib::GUIStateVariables &state, ccalib::Calibrator &calib) {
//        if (ccalib::BeginCard("Calibration", state.fontTitle, 5.5, state.showCalibration)) {
//            ImGuiStyle &style = ImGui::GetStyle();
//            ImGui::AlignTextToFramePadding();
//            ImGui::Text("Rows");
//            ImGui::SameLine(state.widthItemSpacing);
//            ImGui::InputInt("##chkbrd_rows", &calib.checkerboardRows, 1);
//
//            ImGui::AlignTextToFramePadding();
//            ImGui::Text("Cols");
//            ImGui::SameLine(state.widthItemSpacing);
//            ImGui::InputInt("##chkbrd_cols", &calib.checkerboardCols, 1);
//
//            ImGui::AlignTextToFramePadding();
//            ImGui::Text("Size in [m]");
//            ImGui::SameLine(state.widthItemSpacing);
//            ImGui::InputFloat("##chkbrd_size", &calib.checkerboardSize, 0.001f);
//
//            ImGui::AlignTextToFramePadding();
//            ImGui::Text("Calibration");
//            const char *button_text = state.calibrationMode ? "Reset" : "Start";
//            ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(button_text).x -
//                            style.FramePadding.x);
//            if (state.calibrationMode)
//                ccalib::MaterialCancelButton(button_text, !state.calibrationMode && cam.isStreaming());
//            else
//                ccalib::MaterialButton(button_text, !state.calibrationMode && cam.isStreaming());
//            if (ImGui::IsItemClicked()) {
//                state.calibrationMode = !state.calibrationMode;
//                if (state.cameraOn && state.calibrationMode) {
//                    // Reset all parameters
//                    ccalib::CalibrationParameters newCalibParams;
//                    ccalib::CoverageParameters newCoverage;
//                    ccalib::CheckerboardFrame newFrame;
//                    coverage = newCoverage;
//                    frame = newFrame;
//                    calibParams = newCalibParams;
//                    state.undistort = false;
//                    state.snapID = -1;
//                    snapshots.clear();
//                    instanceErrs.clear();
//                }
//            }
//
//            if (!state.cameraOn)
//                state.calibrationMode = false;
//
//            ccalib::EndCard();
//        }
    }

} // namespace ccalib