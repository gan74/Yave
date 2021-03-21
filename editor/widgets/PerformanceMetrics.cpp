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

#include <yave/graphics/graphics.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/graphics/device/LifetimeManager.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static usize to_kb(usize b) {
    return b / 1024;
}

static double to_mb(double b) {
    return b / (1024 * 1024);
}

static double to_mb(usize b) {
    return to_mb(double(b));
}

static const char* memory_type_name(MemoryType type) {
    const char* names[] = {"Generic", "Device local", "Host visible", "Staging"};
    return names[usize(type)];
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
    return true;
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
    ImGui::PlotLines("##averages", _average.values().data(), int(_average.values().size()), int(_average.next_index()), "", 0.0f, _average.max(), ImVec2(ImGui::GetWindowContentRegionWidth(), 80));

    ImGui::Text("Frame time: %.2fms", ms);
    ImGui::SetNextItemWidth(-1);
    ImGui::PlotLines("##frames", _frames.values().data(), int(_frames.values().size()), int(_frames.next_index()), "", 0.0f, _average.max(), ImVec2(ImGui::GetWindowContentRegionWidth(), 80));

    ImGui::Text("%.3u resources waiting deletion", unsigned(lifetime_manager().pending_deletions()));
    ImGui::Text("%.3u active command buffers", unsigned(lifetime_manager().pending_cmd_buffers()));
}

void PerformanceMetrics::draw_memory() {
    static constexpr bool show_heaps = false;

    double total_used = 0;
    double total_allocated = 0;
    for(auto&& [type, heaps] : device_allocator().heaps()) {
        if(show_heaps) {
            ImGui::Bullet();
            ImGui::TextUnformatted(fmt_c_str("Heap [%]", memory_type_name(type.second)));
            ImGui::Indent();
        }
        for(const auto& heap : heaps) {
            usize free = heap->available();
            const usize used = heap->size() - free;
            total_used += to_mb(used);
            total_allocated += to_mb(heap->size());

            if(show_heaps) {
                ImGui::ProgressBar(used / float(heap->size()), ImVec2(0, 0), fmt_c_str("%KB / %KB", to_kb(used), to_kb(heap->size())));
                ImGui::Text("Free blocks: %u", unsigned(heap->free_blocks()));
                ImGui::Spacing();
            }
        }

        if(show_heaps) {
            ImGui::Unindent();
        }
    }

    if(show_heaps) {
        ImGui::Spacing();
        ImGui::Separator();
    }

    double dedicated = 0;
    for(auto&& [type, heap] : device_allocator().dedicated_heaps()) {
        unused(type);
        dedicated += to_mb(heap->allocated_size());
    }
    total_used += dedicated;
    total_allocated += dedicated;

    _memory.push(float(total_used));

    {
        ImGui::Text("Total used: %.1lfMB", total_used);
        ImGui::Text("Dedicated allocations size: %.1lfMB", dedicated);
        ImGui::Text("Total allocated: %.1lfMB", total_allocated);
        ImGui::SetNextItemWidth(-1);
        ImGui::ProgressBar(float(total_used / total_allocated), ImVec2(0, 0), fmt_c_str("%MB / %MB", usize(total_used), usize(total_allocated)));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Max usage: %.1fMB", _memory.max());

        ImGui::SetNextItemWidth(-1);
        ImGui::PlotLines("##memory", _memory.values().data(), int(_memory.values().size()), int(_memory.next_index()), "", 0.0f, _memory.max() * 1.33f, ImVec2(0, 80));
    }

    {
        ImGui::Spacing();
        ImGui::Separator();

        DescriptorSetAllocator& alloc = descriptor_set_allocator();
        usize pools = alloc.pool_count();
        const usize total_sets = pools * DescriptorSetPool::pool_size;
        usize free_sets = alloc.free_sets();
        const usize used_sets = total_sets - free_sets;

        ImGui::Text("Descriptor set layouts: %u", u32(alloc.layout_count()));
        ImGui::Text("Descriptor set pools: %u", u32(pools));

        ImGui::ProgressBar(used_sets / float(total_sets), ImVec2(0, 0), fmt_c_str("% / % sets", used_sets, total_sets));
    }
}

}

