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

static const double ns_to_ms = 1.0 / 1'000'000.0;


template<typename T>
static void display_zone(
        core::Span<CmdTimingRecorder::Event> events,
        core::FlatHashMap<i64, T>& history,
        double gpu_total_ms, double cpu_total_ms,
        bool first = false) {

    if(events.is_empty()) {
        return;
    }

    struct Zone {
        usize start = 0;
        usize end = 0;
        const char* name = nullptr;
        double gpu_ms = 0.0;
        double cpu_ms = 0.0;
    };

    core::Vector<Zone> zones;

    {
        const double tick_to_ms = device_properties().timestamp_period * ns_to_ms;

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
                        auto& zone = zones.emplace_back();
                        zone.start = start;
                        zone.end = i;
                        zone.name = events[zone.start].name.data();
                        zone.cpu_ms = (events[zone.end].cpu_nanos - events[zone.start].cpu_nanos) * ns_to_ms;
                        zone.gpu_ms = (events[zone.end].gpu_timestamp.timestamp() - events[zone.start].gpu_timestamp.timestamp()) * tick_to_ms;
                    }
                break;
            }
        }
    }

    if(ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
        if(sort_specs->SpecsCount > 0) {
            const ImGuiTableColumnSortSpecs& spec = sort_specs->Specs[0];
            if(spec.SortDirection != ImGuiSortDirection_None) {
                auto cmp = [=](double a, double b) {
                    return spec.SortDirection == ImGuiSortDirection_Ascending
                        ? (a > b) : (a < b);
                };
                switch(spec.ColumnIndex) {
                    case 1: // GPU
                        std::sort(zones.begin(), zones.end(), [=](const Zone& a, const Zone& b) { return cmp(a.gpu_ms, b.gpu_ms); });
                    break;

                    case 2: // CPU
                        std::sort(zones.begin(), zones.end(), [=](const Zone& a, const Zone& b) { return cmp(a.cpu_ms, b.cpu_ms); });
                    break;

                    default:
                    break;
                }
            }
        }
    }

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

    const int flags = first ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    for(const Zone& zone : zones) {
        ImGui::PushID(int(zone.start));
        y_defer(ImGui::PopID());

        const i64 id = i64(ImGui::GetID("zone"));
        auto& zone_history = history[id];
        zone_history.update(zone.gpu_ms, zone.cpu_ms);

        const double gpu_ms = zone.gpu_ms;
        const double cpu_ms = zone.cpu_ms;

        {
            imgui::table_begin_next_row(1);
            draw_bg(float(gpu_ms / gpu_total_ms));
            ImGui::Text("%.2f ms", gpu_ms);
        }

        {
            ImGui::TableSetColumnIndex(2);
            draw_bg(float(cpu_ms / cpu_total_ms));
            ImGui::Text("%.2f ms", cpu_ms);
        }

        ImGui::TableSetColumnIndex(0);
        const bool is_leaf = zone.start + 1 == zone.end;
        if(ImGui::TreeNodeEx(zone.name, (is_leaf ? ImGuiTreeNodeFlags_Leaf : 0) | flags, "%s", zone.name)) {
            if(first) {
                gpu_total_ms = gpu_ms;
                cpu_total_ms = cpu_ms;
            }
            const core::Span<CmdTimingRecorder::Event> inner(events.data() + zone.start + 1, zone.end - zone.start - 1);
            display_zone(inner, history, gpu_total_ms, cpu_total_ms);
            ImGui::TreePop();
        }
    }
}





static EngineView* current_view() {
    for(const auto& widget : ui().widgets()) {
        if(EngineView* view = dynamic_cast<EngineView*>(widget.get())) {
            return view;
        }
    }
    return nullptr;
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

    /*const char* display_types[] = {
        "Current time",
        "Average time",
        "Maximum time",
    };
    ImGui::Combo("Display", reinterpret_cast<int*>(&_display_type), display_types, 3);

    ImGui::SameLine();*/

    if(ImGui::Button("Clear history")) {
        _history.clear();
    }

    if(ImGui::BeginChild("##tree")) {
        const ImGuiTableFlags table_flags =
                ImGuiTableFlags_Sortable |
                ImGuiTableFlags_SortTristate |
                ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_RowBg;

        if(ImGui::BeginTable("##timetable", 3, table_flags)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_NoResize, 100.0f);
            ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_NoResize, 100.0f);
            ImGui::TableHeadersRow();

            display_zone(time_rec->events(), _history, 0.0, 0.0, true);

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

}

}

