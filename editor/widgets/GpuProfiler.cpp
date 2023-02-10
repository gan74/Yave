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




static bool display_event(const CmdTimingRecorder::Event& start, const CmdTimingRecorder::Event& end, bool leaf) {
    const double period = device_properties().timestamp_period;
    const double ms = core::Duration::nanoseconds(end.query.get() - start.query.get()).to_millis() * period;

    int flags = 0;
    if(leaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    return ImGui::TreeNodeEx(start.name.data(), flags, "%s (%.2f ms)", start.name.data(), ms);
}

static void display_zone(core::Span<CmdTimingRecorder::Event> events) {
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
                    if(display_event(events[start], events[i], start + 1 == i)) {
                        display_zone(core::Span<CmdTimingRecorder::Event>(events.data() + start + 1, i - start - 1));
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

    {
        const u64 start = events[0].query.get();
        const u64 end = events[events.size() - 1].query.get();
        const float period = device_properties().timestamp_period;
        ImGui::Text("Total: %.2f ms", float(core::Duration::nanoseconds(end - start).to_millis() * period));
    }

    display_zone(events);

}

}

