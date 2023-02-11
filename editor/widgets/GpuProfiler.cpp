/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "GpuProfiler.h"

#include "EngineView.h"

#include <yave/graphics/commands/CmdTimingRecorder.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <editor/UiManager.h>

#include <editor/utils/ui.h>

namespace editor {

static EngineView* current_view() {
    for(const auto& widget : ui().widgets()) {
        if(EngineView* view = dynamic_cast<EngineView*>(widget.get())) {
            return view;
        }
    }
    return nullptr;
}


static const double ns_to_ms = 1.0 / 1'000'000.0;

static bool display_event(const CmdTimingRecorder::Event& start, const CmdTimingRecorder::Event& end, double gpu_total, double cpu_total, ImGuiTreeNodeFlags flags) {
    imgui::table_begin_next_row(1);

    const math::Vec4 color = math::Vec4(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram)) * math::Vec4(1.0f, 1.0f, 1.0f, 0.5f);
    const u32 color_u32 = ImGui::GetColorU32(color);

    auto draw_bg = [=](float ratio) {
        const float width = ImGui::GetContentRegionAvail().x * std::min(1.0f, ratio);
        if(width <= 0.5f) {
            return;
        }
        const math::Vec2 size(width, ImGui::GetTextLineHeight());
        const math::Vec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + size, color_u32);
    };

    {
        const double tick_to_ms = device_properties().timestamp_period * ns_to_ms;
        const double gpu = (end.gpu_ticks.timestamp() - start.gpu_ticks.timestamp()) * tick_to_ms;
        draw_bg(float(gpu / gpu_total));
        ImGui::Text("%.2f ms", gpu);
    }

    ImGui::TableSetColumnIndex(2);

    {
        const double cpu = (end.cpu_nanos - start.cpu_nanos) * ns_to_ms;
        draw_bg(float(cpu / cpu_total));
        ImGui::Text("%.2f ms", cpu);
    }

    ImGui::TableSetColumnIndex(0);

    return ImGui::TreeNodeEx(start.name.data(), flags, "%s", start.name.data());
}

static void display_zone(core::Span<CmdTimingRecorder::Event> events, double gpu_total, double cpu_total, bool first = false) {
    if(events.is_empty()) {
        return;
    }

    usize start = 0;
    usize depth = 0;
    for(usize i = 0; i != events.size(); ++i) {
        switch(events[i].type) {
            case CmdTimingRecorder::EventType::BeginZone:
                if(!depth) {
                    start = i;
                }
                ++depth;
            break;

            case CmdTimingRecorder::EventType::EndZone:
                if(!depth) {
                    return; // for safety
                }
                if(--depth == 0) {
                    ImGui::PushID(int(start));

                    int flags = 0;
                    {
                        flags |= start + 1 == i ? ImGuiTreeNodeFlags_Leaf : 0;
                        flags |= first ? ImGuiTreeNodeFlags_DefaultOpen : 0;
                    }
                    if(display_event(events[start], events[i], gpu_total, cpu_total, flags)) {
                        display_zone(core::Span<CmdTimingRecorder::Event>(events.data() + start + 1, i - start - 1), gpu_total, cpu_total);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            break;
        }
    }
}



GpuProfiler::GpuProfiler() :Widget(ICON_FA_CLOCK " GPU Profiler") {
}

void GpuProfiler::on_gui() {
    EngineView* current = current_view();
    if(!current) {
        ImGui::TextUnformatted("No engine view to profile");
        return;
    }

    CmdTimingRecorder* time_rec = current->timing_recorder();
    if(!time_rec || time_rec->events().size() < 2) {
        ImGui::TextUnformatted("No recorder timings");
        return;
    }

    const auto events = time_rec->events();

    const double tick_to_ms = device_properties().timestamp_period * ns_to_ms;
    const double gpu_total = (events[events.size() - 1].gpu_ticks.timestamp() - events[0].gpu_ticks.timestamp()) * tick_to_ms;
    const double cpu_total = (events[events.size() - 1].cpu_nanos - events[0].cpu_nanos) * ns_to_ms;
    ImGui::Text("Total GPU: %.2f ms", gpu_total);
    ImGui::Text("Total CPU: %.2f ms", cpu_total);

    if(ImGui::BeginChild("##tree")) {
        const ImGuiTableFlags table_flags =
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_RowBg;

        if(ImGui::BeginTable("##timetable", 3, table_flags)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_NoResize);
            ImGui::TableHeadersRow();

            display_zone(events, gpu_total, cpu_total, true);

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

}

}

