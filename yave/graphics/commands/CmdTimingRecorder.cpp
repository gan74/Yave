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

#include "CmdTimingRecorder.h"

#include "CmdBufferRecorder.h"

namespace yave {

CmdTimingRecorder::CmdTimingRecorder(const CmdBufferRecorderBase& recorder) : _query_pool(recorder.vk_cmd_buffer()) {
}

VkCommandBuffer CmdTimingRecorder::vk_cmd_buffer() const {
    return _query_pool.vk_cmd_buffer();
}

bool CmdTimingRecorder::is_data_ready() const {
    return _query_pool.all_query_ready();
}

core::Span<CmdTimingRecorder::Event> CmdTimingRecorder::events() const {
    if(!is_data_ready()) {
        return {};
    }

    return _events.locked([](auto&& events) -> core::Span<Event> { return events; });
}

void CmdTimingRecorder::begin_zone(const char* name) {
    _events.locked([&](auto&& events) {
        events.emplace_back(EventType::BeginZone, name, _query_pool.query(PipelineStage::BeginOfPipe), _cpu.elapsed().to_nanos());
    });
}

void CmdTimingRecorder::end_zone() {
    _events.locked([&](auto&& events) {
        events.emplace_back(EventType::EndZone, "", _query_pool.query(PipelineStage::EndOfPipe), _cpu.elapsed().to_nanos());
    });
}

}


