/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "SettingsPanel.h"

#include <editor/context/EditorContext.h>

#include <imgui/yave_imgui.h>

namespace editor {

SettingsPanel::SettingsPanel(ContextPtr cptr) : Widget(ICON_FA_COG " Settings"), ContextLinked(cptr) {
}



static void keybox(const char* name, Key& key) {
    char k[2] = {char(key), 0};

    ImGui::PushItemWidth(24);
    ImGuiInputTextCallback callback = [](ImGuiInputTextCallbackData* data) { data->CursorPos = 0; return 0; };
    ImGui::InputText(name, k, sizeof(k), ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways, callback);

    if(std::isalpha(k[0])) {
        key = Key(k[0]);
    }

    ImGui::PopItemWidth();
}

static void camera_settings(ContextPtr ctx) {
    CameraSettings& cam = ctx->settings().camera();

    if(ImGui::CollapsingHeader("FPS Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        keybox("Forward", cam.move_forward);
        ImGui::SameLine();
        keybox("Backward", cam.move_backward);

        keybox("Left", cam.move_left);
        ImGui::SameLine();
        keybox("Right", cam.move_right);

        ImGui::SliderFloat("Sensitivity##FPS", &cam.fps_sensitivity, 0.1f, 10.0f, "%.1f", 2.0f);
    }

    if(ImGui::CollapsingHeader("Houdini Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Trackball sensitivity##Houdini", &cam.trackball_sensitivity, 0.1f, 10.0f, "%.1f", 2.0f);
        ImGui::SliderFloat("Dolly sensitivity##Houdini", &cam.dolly_sensitivity, 0.1f, 10.0f, "%.1f", 2.0f);
    }

    ImGui::SliderFloat("Near plane distance", &cam.z_near, 0.01f, 10.0f, "%.2f", 2.0f);
    ImGui::SliderFloat("Field of view", &cam.fov, 30.0f, 170.0f, "%.0f", 2.0f);

    keybox("Center on selected object", cam.center_on_obj);
}

static void ui_settings(ContextPtr ctx) {
    UiSettings& ui = ctx->settings().ui();

    keybox("Change gizmo mode", ui.change_gizmo_mode);
    ImGui::SameLine();
    keybox("Change gizmo space", ui.change_gizmo_space);
}

static void perf_settings(ContextPtr ctx) {
    PerfSettings& perf = ctx->settings().perf();

    int capture_frames = perf.capture_frames;
    ImGui::InputInt("Number of frames to capture", &capture_frames);
    ImGui::Checkbox("Capture indefinitely", &perf.capture_forever);

    if(capture_frames > 1) {
        perf.capture_frames = capture_frames;
    }
}

void SettingsPanel::paint_ui(CmdBufferRecorder&, const FrameToken&) {


    struct SettingCategory {
        const char* name;
        void (*func)(ContextPtr);
    };

    const std::array<SettingCategory, 3> categories = {{
        {ICON_FA_VIDEO " Camera", camera_settings},
        {ICON_FA_WINDOW_RESTORE " Interface", ui_settings},
        {ICON_FA_STOPWATCH " Performance", perf_settings}
    }};

    {
        ImGui::BeginChild("##categories", ImVec2(150, -24));

        for(usize i = 0; i != categories.size(); ++i) {
            const bool selected = _category == i;
            if(ImGui::Selectable(categories[i].name, selected)) {
                _category = i;
            }
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    {
        ImGui::BeginChild("##settings", ImVec2(0, -24));
        categories[_category].func(context());
        ImGui::EndChild();
    }

    if(ImGui::Button("Reset to defaults")) {
        context()->settings() = Settings(false);
    }
}

}

