//
// Created by andya on 23.01.20.
//

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_widgets.h"
#include "imgui_extensions.h"
#include "structures.h"
#include "camera.h"

#include <vector>

namespace ccalib {

    void CameraCard (ccalib::GUIStateVariables &state, ccalib::Camera &cam, ccalib::CameraParameters &camParams) {
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

} // namespace ccalib