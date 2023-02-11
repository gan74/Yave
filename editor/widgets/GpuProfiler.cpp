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

static bool display_event(const CmdTimingRecorder::Event& start, const CmdTimingRecorder::Event& end, bool leaf, double gpu_total, double cpu_total) {
    int flags = 0;
    if(leaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    imgui::table_begin_next_row(1);

    {
        unused(gpu_total);
        const double tick_to_ms = device_properties().timestamp_period * ns_to_ms;
        const double gpu = (end.gpu_ticks.timestamp() - start.gpu_ticks.timestamp()) * tick_to_ms;
        //ImGui::ProgressBar(float(gpu / gpu_total), ImVec2(0.0f, 0.0f), fmt_c_str("% ms", std::round(gpu * 100.0) / 100.0));
        ImGui::Text("%.2f ms", gpu);
    }

    ImGui::TableSetColumnIndex(2);

    {
        unused(cpu_total);
        const double cpu = (end.cpu_nanos - start.cpu_nanos) * ns_to_ms;
        //ImGui::ProgressBar(float(cpu / cpu_total), ImVec2(0.0f, 0.0f), fmt_c_str("% ms", std::round(cpu * 100.0) / 100.0));
        ImGui::Text("%.2f ms", cpu);
    }

    ImGui::TableSetColumnIndex(0);

    return ImGui::TreeNodeEx(start.name.data(), flags, "%s", start.name.data());
}

static void display_zone(core::Span<CmdTimingRecorder::Event> events, double gpu_total, double cpu_total) {
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
                    if(display_event(events[start], events[i], start + 1 == i, gpu_total, cpu_total)) {
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

            display_zone(events, gpu_total, cpu_total);

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

}

}

