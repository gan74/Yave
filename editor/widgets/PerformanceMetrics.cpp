/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "PerformanceMetrics.h"

#include <yave/graphics/utils.h>
#include <yave/graphics/device/LifetimeManager.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

PerformanceMetrics::PerformanceMetrics() : Widget("Performance", ImGuiWindowFlags_AlwaysAutoResize) {
    std::fill(_frames.begin(), _frames.end(), 0.0f);
    std::fill(_average.begin(), _average.end(), 0.0f);
}

void PerformanceMetrics::draw_gui() {
    const float ms = float(_timer.reset().to_millis());
    const float avg = float(_total / _frames.size());

    _total = _total - _frames[_current_frame] + ms;
    _frames[_current_frame] = ms;
    _current_frame = (_current_frame + 1) % _frames.size();

    if(!_current_frame) {
        _average[_current_average] = avg;
        _current_average = (_current_average + 1) % _average.size();

        double sum = 0;
        usize samples = 0;
        for(float t : _average) {
            if(t > 0.0f) {
                sum += t;
                ++samples;
            }
        }
        _max = (samples ? float(sum / samples) : 16.0f) * 2.0f;
    }

    ImGui::Text("Average time: %.2fms", avg);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##averages", _average.data(), _average.size(), _current_average, "", 0.0f, _max, ImVec2(ImGui::GetWindowContentRegionWidth(), 80));

    ImGui::Text("Frame time: %.2fms", ms);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##frames", _frames.data(), _frames.size(), _current_frame, "", 0.0f, _max, ImVec2(ImGui::GetWindowContentRegionWidth(), 80));

    ImGui::Text("%.3u resources waiting deletion", unsigned(lifetime_manager(app_device()).pending_deletions()));
    ImGui::Text("%.3u active command buffers", unsigned(lifetime_manager(app_device()).pending_cmd_buffers()));
}

}

