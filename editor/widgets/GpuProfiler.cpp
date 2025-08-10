/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <yave/graphics/commands/CmdTimestampPool.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/device/Instance.h>

#include <editor/UiManager.h>

#include <editor/utils/ui.h>

namespace editor {

static const double ns_to_ms = 1.0 / 1'000'000.0;

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

    CmdTimestampPool* ts_pool = current->timestamp_pool();
    if(!ts_pool || ts_pool->is_empty()) {
        ImGui::TextUnformatted("No recorder timings");
        return;
    }

    if(ImGui::Button("Clear history")) {
        _history.clear();
    }

    if(instance_params().validation_layers) {
        ImGui::SameLine();
        ImGui::TextColored(imgui::error_text_color, "(Debug layers enabled)");
    }

    if(ImGui::Checkbox("Display hierarchy", &_display_tree)) {
        _history.clear();
    }

    if(ImGui::BeginChild("##hierarchy")) {
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

            core::Vector<CmdTimestampPool::TimedZone> zones;
            ts_pool->for_each_zone([&](const auto& zone) { zones.emplace_back(zone); }, true);


            bool as_tree = _display_tree;
            if(ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if(sort_specs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs& spec = sort_specs->Specs[0];
                    if(spec.SortDirection != ImGuiSortDirection_None) {
                        as_tree = false;
                        auto cmp = [=](auto a, auto b) { return spec.SortDirection == ImGuiSortDirection_Ascending ? (a > b) : (a < b); };
                        switch(spec.ColumnIndex) {
                            case 1: // GPU
                                std::sort(zones.begin(), zones.end(), [=](const auto& a, const auto& b) { return cmp(a.gpu_nanos, b.gpu_nanos); });
                            break;

                            case 2: // CPU
                                std::sort(zones.begin(), zones.end(), [=](const auto& a, const auto& b) { return cmp(a.cpu_nanos, b.cpu_nanos); });
                            break;

                            default:
                            break;
                        }
                    }
                }
            }



            float cpu_total_ms = 0.0f;
            float gpu_total_ms = 0.0f;
            for(usize i = 0; i < zones.size(); ++i) {
                cpu_total_ms += float(zones[i].cpu_nanos * ns_to_ms);
                gpu_total_ms += float(zones[i].gpu_nanos * ns_to_ms);
                i += zones[i].contained_zones;
            }


            /*for(usize i = 0; i < zones.size(); ++i) {
                const auto& zone = zones[i];
                const double gpu_ms = double(zone.gpu_nanos * ns_to_ms);
                const double cpu_ms = double(zone.cpu_nanos * ns_to_ms);

                auto& zone_history = _history[i64(ImGui::GetID("zone"))];
                zone_history.update(gpu_ms, cpu_ms);
            }*/


            const u32 color_u32 = ImGui::GetColorU32(ImGuiCol_PlotHistogram, 0.4f);
            auto draw_bg = [=](float ratio) {
                const float width = ImGui::GetContentRegionAvail().x * std::min(1.0f, ratio);
                if(width <= 0.5f) {
                    return;
                }
                const ImVec2 size = ImVec2(width, ImGui::GetTextLineHeight());
                const ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + size, color_u32);
            };

            core::SmallVector<u32, 16> indents;
            for(usize i = 0; i < zones.size(); ++i) {
                const auto& zone = zones[i];

                const double gpu_ms = double(zone.gpu_nanos * ns_to_ms);
                const double cpu_ms = double(zone.cpu_nanos * ns_to_ms);

                imgui::table_begin_next_row(0);

                const bool has_children = as_tree && zone.contained_zones;
                const char* name = (!as_tree && zone.contained_zones) ? fmt_c_str("{} *", zone.name) : zone.name.data();
                const bool open = ImGui::TreeNodeEx(fmt_c_str("{}##{}", name, i), has_children ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf);
                if(open) {
                    ImGui::TreePop();
                } else if(as_tree) {
                    i += zone.contained_zones;
                }

                ImGui::TableSetColumnIndex(1);

                if(gpu_ms >= 0.0) {
                    draw_bg(float(gpu_ms / gpu_total_ms));
                    ImGui::Text("%.2f ms", gpu_ms);

                    /*if(ImGui::BeginItemTooltip()) {
                        ImGui::Text("avg: %.2f ms", zone_history.gpu_ms_sum / zone_history.sample_count);
                        ImGui::Text("max: %.2f ms", zone_history.gpu_ms_max);
                        ImGui::EndTooltip();
                    }*/
                }

                ImGui::TableSetColumnIndex(2);

                {
                    draw_bg(float(cpu_ms / cpu_total_ms));
                    ImGui::Text("%.2f ms", cpu_ms);

                    /*if(ImGui::BeginItemTooltip()) {
                        ImGui::Text("avg: %.2f ms", zone_history.cpu_ms_sum / zone_history.sample_count);
                        ImGui::Text("max: %.2f ms", zone_history.cpu_ms_max);
                        ImGui::EndTooltip();
                    }*/
                }

                if(as_tree) {
                    if(!indents.is_empty() && --indents.last() == 0) {
                        ImGui::PopID();
                        indents.pop();
                        ImGui::Unindent();
                    }

                    if(has_children && open) {
                        ImGui::PushID(zone.name.data());
                        indents.push_back(zone.contained_zones);
                        ImGui::Indent();
                    }
                }
            }

            for(usize i = 0; i != indents.size(); ++i) {
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

}

}

