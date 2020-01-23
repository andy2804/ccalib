//
// Created by andya on 23.01.20.
//

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_extensions.h"

#include <opencv2/opencv.hpp>

#include <vector>

namespace ccalib {

    void ToggleButton(const char *str_id, bool *v, const bool focus) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight() * 0.8f;
        float width = height * 1.8f;
        float radius = height * 0.50f;
        p.y += 0.1f * ImGui::GetFrameHeight();

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if (ImGui::IsItemClicked())
            *v = !*v;

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext &g = *GImGui;
        float ANIM_SPEED = 0.08f;
        if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
        {
            float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
            t = *v ? (t_anim) : (1.0f - t_anim);
        }

        // Draw rectangle if focused
        ImU32 col_bg;
        if (focus) {
            float padding = 1.7f;
            float margin = 0.0f;
            float ANIM_SPEED = 0.32f;
            float t_anim = cos(g.LastActiveIdTimer / ANIM_SPEED);
            col_bg = ImGui::GetColorU32(
                    ImLerp(ImVec4(0.56f, 0.83f, 0.26f, 1.0f), ImVec4(0.72f, 0.91f, 0.42f, 1.0f), t_anim));

            draw_list->AddRect(ImVec2(p.x - padding - margin, p.y - padding - margin),
                               ImVec2(p.x + width + padding + margin, p.y + height + padding + margin), col_bg,
                               height * 0.5f,
                               ImDrawCornerFlags_All, padding * 2);
        }

        if (ImGui::IsItemHovered())
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
        else
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f,
                                   IM_COL32(255, 255, 255, 255));
    }

    void CoveredBar(const float &start, const float &stop, const float &indicator) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight() * 0.25f;
        float width = ImGui::GetContentRegionAvailWidth();
        p.y += 0.375f * ImGui::GetFrameHeight();

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height),
                                 ImGui::GetColorU32(ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
        if (stop > start) {
            draw_list->AddRectFilled(ImVec2(p.x + std::max(0.0f, start) * width, p.y),
                                     ImVec2(p.x + std::min(1.0f, stop) * width, p.y + height),
                                     ImGui::GetColorU32(ImVec4(0.56f, 0.83f, 0.26f, 1.0f)), height * 0.5f);
        }

        if (indicator >= 0) {
            draw_list->AddCircleFilled(ImVec2(p.x + indicator * width, p.y + height / 2.0f), height + 1.5f,
                                       ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.1f)));
            draw_list->AddCircleFilled(ImVec2(p.x + indicator * width, p.y + height / 2.0f), height,
                                       ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
        }

        ImGui::InvisibleButton("##covered_bar", ImVec2(width, ImGui::GetFrameHeight()));
    }

    bool MaterialButton(const char *label, bool focus, const ImVec2 &size) {
        // Get Position and Drawlist
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();


        // Draw button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.56f, 0.83f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.75f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        bool clicked = ImGui::Button(label, size);
        float height = ImGui::GetItemRectSize().y;
        float width = ImGui::GetItemRectSize().x;

        ImGui::PopStyleColor(3);

        // Draw rectangle if focused
        if (focus) {
            ImU32 col_bg;
            ImGuiContext &g = *GImGui;
            float padding = 1.0f;
            float ANIM_SPEED = 0.32f;
            float t_anim = cos(g.LastActiveIdTimer / ANIM_SPEED);
            col_bg = ImGui::GetColorU32(
                    ImLerp(ImVec4(0.56f, 0.83f, 0.26f, 1.0f), ImVec4(0.72f, 0.91f, 0.42f, 1.0f), t_anim));

            draw_list->AddRect(ImVec2(p.x - padding, p.y - padding),
                               ImVec2(p.x + width + padding, p.y + height + padding), col_bg,
                               ImGui::GetStyle().FrameRounding,
                               ImDrawCornerFlags_All, padding * 2);
        }

        return clicked;
    }

    bool BeginCard(const char *label, ImFont *title_font, const float &item_height, bool &visible) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = ImGui::GetID(label);
        ImVec2 item_size = ImVec2(ImGui::GetContentRegionAvailWidth(),
                                  ImGui::GetFrameHeightWithSpacing() * item_height);
        ImVec2 label_size = title_font->CalcTextSizeA(title_font->FontSize, FLT_MAX, -1.0f, label);
        label_size.x = item_size.x;
        label_size.y += style.ItemSpacing.y * 2;

        float t = visible ? 1.0f : 0.0f;
        float ANIM_SPEED = 0.16f;
        if (g.LastActiveId == id) {
            float t_anim = g.LastActiveIdTimer / ANIM_SPEED;
            if (t_anim > 1.0f) {
                ImGui::ClearActiveID();
                t_anim = 1.0f;
            }
            t = visible ? (t_anim) : (1.0f - t_anim);
        }
        item_size = ImLerp(label_size, item_size, t);

        // Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
        ImVec2 size = ImGui::CalcItemSize(item_size, ImGui::CalcItemWidth(),
                                          ImGui::GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
        ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
        ImRect bb(frame_bb.Min, frame_bb.Max);
        window->DC.LastItemRect = bb; // Forward storage
        g.NextItemData.ClearFlags();

        if (!ImGui::IsRectVisible(bb.Min, bb.Max)) {
            ImGui::ItemSize(bb.GetSize(), style.FramePadding.y);
            ImGui::ItemAdd(bb, 0, &frame_bb);
            return false;
        }

        const ImVec2 p = ImGui::GetCursorPos();
        ImGui::InvisibleButton(label, label_size);
        if (ImGui::IsItemClicked()) {
            ImGui::SetActiveID(id, window);
            visible = !visible;
        }
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_ChildBg, std::max(0.5f, t));
//    ImU32 col_bg = ImGui::GetColorU32(
//            ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_Header), ImGui::GetStyleColorVec4(ImGuiCol_ChildBg), t));
        if (ImGui::IsItemHovered() && !visible && t == 0.0f) {
            col_bg = ImGui::GetColorU32(ImGuiCol_ChildBg);
        }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, col_bg, style.ChildRounding);

        ImGui::SetCursorPos(p);
        ImGui::BeginChild(id, frame_bb.GetSize(), true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        ImGui::PushFont(title_font);
        ImGui::Text("%s", label);

        ImGui::PushItemWidth(-1);

        ImGui::PopFont();
        ImGui::Separator();
        return true;
    }

    void EndCard() {
        ImGuiWindow *parent_window = ImGui::GetCurrentWindow()->ParentWindow;
        const ImRect bb = parent_window->DC.LastItemRect;
        const ImGuiStyle &style = ImGui::GetStyle();

        ImGui::PopItemWidth();

        ImGui::EndChild();
        ImGui::PopStyleColor(1);

        ImGui::SameLine();
        parent_window->DC.CursorPos = bb.Min;
        ImGui::ItemSize(bb, style.FramePadding.y);
    }

    void drawRectangle(const std::vector<cv::Point2f> &corners, const ImVec4 &color, const float &thickness, bool focus) {
        // Get Position and Drawlist
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImGuiContext &g = *GImGui;
        ImU32 col_bg;

        // Draw rectangle if focused
        if (focus) {
            float ANIM_SPEED = 0.32f;
            float t_anim = cos(g.LastActiveIdTimer / ANIM_SPEED);
            const ImVec4 color_dark = ImVec4(ImSaturate(color.x * 0.8f), ImSaturate(color.y * 0.8f),
                                             ImSaturate(color.z * 0.8f), ImSaturate(color.w * 0.8f));
            const ImVec4 color_bright = ImVec4(ImSaturate(color.x * 1.2f), ImSaturate(color.y * 1.2f),
                                               ImSaturate(color.z * 1.2f), ImSaturate(color.w * 1.2f));
            col_bg = ImGui::GetColorU32(ImLerp(color_dark, color_bright, t_anim));
        } else
            col_bg = ImGui::GetColorU32(color);

        // Draw Rectangle
        ImVec2 im_corners[corners.size()];
        for (int i = 0; i < corners.size(); i++) {
            im_corners[i].x = corners[i].x;
            im_corners[i].y = corners[i].y;
        }
        draw_list->AddPolyline(im_corners, IM_ARRAYSIZE(im_corners), col_bg, true, thickness);
//    draw_list->AddQuad(corners[0], corners[1], corners[2], corners[3], col_bg, thickness);
    }

    void drawPoints(const std::vector<cv::Point2f> &corners, const ImVec4 &color, const float &size, bool focus) {
        // Get Position and Drawlist
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImGuiContext &g = *GImGui;
        ImU32 col_bg;

        // Draw rectangle if focused
        if (focus) {
            float ANIM_SPEED = 0.32f;
            float t_anim = cos(g.LastActiveIdTimer / ANIM_SPEED);
            const ImVec4 color_dark = ImVec4(ImSaturate(color.x * 0.8f), ImSaturate(color.y * 0.8f),
                                             ImSaturate(color.z * 0.8f), ImSaturate(color.w * 0.8f));
            const ImVec4 color_bright = ImVec4(ImSaturate(color.x * 1.2f), ImSaturate(color.y * 1.2f),
                                               ImSaturate(color.z * 1.2f), ImSaturate(color.w * 1.2f));
            col_bg = ImGui::GetColorU32(ImLerp(color_dark, color_bright, t_anim));
        } else
            col_bg = ImGui::GetColorU32(color);

        // Draw Rectangle
        for (int i = 0; i < corners.size(); i++) {
            draw_list->AddCircleFilled(ImVec2(corners[i].x, corners[i].y), size, col_bg);
            draw_list->AddCircle(ImVec2(corners[i].x, corners[i].y), size * 4.0f, col_bg, 12, size);
            draw_list->AddText(ImVec2(corners[i].x + size * 5.0f, corners[i].y + size * 5.0f), col_bg,
                               std::to_string(i + 1).c_str());
        }
    }

} // namespace ccalib