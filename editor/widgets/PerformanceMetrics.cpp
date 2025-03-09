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

#include "PerformanceMetrics.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/graphics/device/LifetimeManager.h>
#include <yave/graphics/device/DescriptorLayoutAllocator.h>

#include <editor/utils/ui.h>

#include <y/utils/format.h>

namespace editor {

static double to_mb(double b) {
    return b / (1024 * 1024);
}

static double to_mb(u64 b) {
    return to_mb(double(b));
}

PerformanceMetrics::PlotData::PlotData(core::Duration total_duration, usize size) : _data(size), _duration(total_duration.to_secs() / size) {
    std::fill_n(_data.data(), size, 0.0f);
}

void PerformanceMetrics::PlotData::push(float value) {
    if(_timer.elapsed().to_secs() >= _duration) {
        _timer.reset();
        advance();
    }
    _total -= _data[_current];
    _max = std::max(_max, value);
    _data[_current] = std::max(_data[_current], value);
    _total += _data[_current];
}

float PerformanceMetrics::PlotData::last() {
    return _data[_current];
}

core::Span<float> PerformanceMetrics::PlotData::values() const {
    return _data;
}

usize PerformanceMetrics::PlotData::value_count() const {
    return _full ? _data.size() : _current;
}

usize PerformanceMetrics::PlotData::current_index() const {
    return _current;
}

usize PerformanceMetrics::PlotData::next_index() const {
    return (_current + 1) % _data.size();
}

float PerformanceMetrics::PlotData::max() const {
    return _max;
}

float PerformanceMetrics::PlotData::average() const {
    return float(_total / value_count());
}

void PerformanceMetrics::PlotData::advance() {
    if(++_current >= _data.size()) {
        _current -= _data.size();
        _full = true;
    }
    _data[_current] = 0.0f;
    if(_full) {
        _max = 0.0f;
        _total = 0.0f;
        for(usize i = 0; i != _data.size(); ++i) {
            _max = std::max(_max, _data[i]);
            _total += _data[i];
        }
    }
}



PerformanceMetrics::PerformanceMetrics() :
        Widget("Performance"),
        _frames(core::Duration::seconds(1)) {
}

bool PerformanceMetrics::before_gui() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, 0.0f), ImVec2(-1.0f, std::numeric_limits<float>::max()));
    return Widget::before_gui();
}

void PerformanceMetrics::on_gui() {
    if(ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        draw_timings();
        ImGui::Unindent();
    }
    if(ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        draw_memory();
        ImGui::Unindent();
    }
}

void PerformanceMetrics::draw_timings() {
    const float ms = float(_timer.reset().to_millis());
    _average.push(ms);
    _frames.push(ms);

    ImGui::Text("Max time: %.2fms", _average.max());
    ImGui::Text("Average time: %.2fms", _frames.average());
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##averages", _average.values().data(), int(_average.values().size()), int(_average.next_index()), "", 0.0f, _average.max(), ImVec2(ImGui::GetContentRegionAvail().x, 80));

    ImGui::Text("Frame time: %.2fms", ms);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##frames", _frames.values().data(), int(_frames.values().size()), int(_frames.next_index()), "", 0.0f, _average.max(), ImVec2(ImGui::GetContentRegionAvail().x, 80));

    ImGui::Text("%.3u resources waiting deletion", unsigned(lifetime_manager().pending_deletions()));
    ImGui::Text("%.3u active command buffers", unsigned(lifetime_manager().pending_cmd_buffers()));
}

void PerformanceMetrics::draw_memory() {
    double used_per_type_mb[4] = {};
    double allocated_per_type_mb[4] = {};
    for(const auto& heaps : device_allocator().heaps()) {
        for(const auto& heap : heaps) {
            u64 free = heap->available();
            const u64 used = heap->size() - free;
            used_per_type_mb[uenum(heap->memory_type())] += to_mb(used);
            allocated_per_type_mb[uenum(heap->memory_type())] += to_mb(heap->size());
        }
    }

    double dedicated_mb = 0.0;
    usize dedicated_count = 0;
    for(const auto& heap : device_allocator().dedicated_heaps()) {
        const double mb = to_mb(heap->allocated_size());
        dedicated_mb += mb;
        dedicated_count += heap->allocation_count();
        used_per_type_mb[uenum(heap->memory_type())] += mb;
        allocated_per_type_mb[uenum(heap->memory_type())] += mb;
    }

    auto progress_bar = [](double used, double allocated) {
        ImGui::ProgressBar(float(used / allocated), ImVec2(0, 0), fmt_c_str("{}MB / {}MB", u64(used), u64(allocated)));
    };


    double total_used_mb = 0.0;
    double total_allocated_mb = 0.0;
    for(usize i = 0; i != 4; ++i) {
        total_used_mb += used_per_type_mb[i];
        total_allocated_mb += allocated_per_type_mb[i];
    }

    _memory.push(float(total_used_mb));

    {
        ImGui::Text("Total used: %.1lfMB", total_used_mb);
        ImGui::Text("Dedicated allocation: %.1lfMB across %u allocations", dedicated_mb, unsigned(dedicated_count));
        ImGui::Text("Total allocated: %.1lfMB", total_allocated_mb);
        ImGui::SetNextItemWidth(-1);
        progress_bar(total_used_mb, total_allocated_mb);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Max usage: %.1fMB", _memory.max());

        ImGui::SetNextItemWidth(-1);
        ImGui::PlotLines("##memory", _memory.values().data(), int(_memory.values().size()), int(_memory.next_index()), "", 0.0f, _memory.max() * 1.33f, ImVec2(0, 80));
    }

    {
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Descriptor set layouts: %u", u32(layout_allocator().layout_count()));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Show heaps", &_show_heaps);

    if(_show_heaps) {
        ImGui::Indent();
        for(usize i = 0; i != 4; ++i) {
            if(allocated_per_type_mb[i] == 0) {
                continue;
            }
            ImGui::Bullet();
            ImGui::TextUnformatted(memory_type_name(MemoryType(i)));
            progress_bar(used_per_type_mb[i], allocated_per_type_mb[i]);

        }
        ImGui::Unindent();
    }
}

}
