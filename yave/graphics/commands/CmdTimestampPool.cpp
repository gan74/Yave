/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "CmdTimestampPool.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/DebugUtils.h>

namespace yave {

static constexpr u32 timestamp_pool_size = 1 << 10;

static VkHandle<VkQueryPool> create_pool(VkCommandBuffer cmd_buffer) {
    VkQueryPoolCreateInfo create_info = vk_struct();
    {
        create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        create_info.queryCount = timestamp_pool_size;
    }

    VkHandle<VkQueryPool> pool;
    vk_check(vkCreateQueryPool(vk_device(), &create_info, vk_allocation_callbacks(), pool.get_ptr_for_init()));
    vkCmdResetQueryPool(cmd_buffer, pool, 0, timestamp_pool_size);

    return pool;
}




CmdTimestampPool::CmdTimestampPool(const CmdBufferRecorderBase& recorder) :
        _pool(create_pool(recorder.vk_cmd_buffer())),
        _cmd_buffer(recorder.vk_cmd_buffer()),
        _results(std::make_unique<u64[]>(timestamp_pool_size)) {
}

CmdTimestampPool::~CmdTimestampPool() {
    if(_pool) {
        destroy_graphic_resource(std::move(_pool));
    }
}

VkCommandBuffer CmdTimestampPool::vk_cmd_buffer() const {
    return _cmd_buffer;
}

bool CmdTimestampPool::is_empty() const {
    return !_query_index;
}

bool CmdTimestampPool::is_ready(bool wait) const {
    if(!_pool) {
        return true;
    }

    const VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : 0);
    return VK_NOT_READY != vk_check(vkGetQueryPoolResults(vk_device(), _pool, 0, _query_index, sizeof(u64) * _query_index, _results.get(), sizeof(u64), flags));
}

u32 CmdTimestampPool::alloc_query(PipelineStage stage) {
    y_debug_assert(_pool);

    if(_query_index >= timestamp_pool_size) {
        return u32(-1);
    }

    const u32 index = _query_index++;
    vkCmdWriteTimestamp(_cmd_buffer, VkPipelineStageFlagBits(stage), _pool, index);
    return index;
}

u32 CmdTimestampPool::begin_zone(core::String name) {
    const u32 index = u32(_zones.size());

    auto& zone = _zones.emplace_back();
    {
        zone.name = std::move(name);
        zone.cpu_nanos = _chrono.elapsed().to_nanos();
        zone.start_query = alloc_query(PipelineStage::BeginOfPipe);
    }

    return index;
}

void CmdTimestampPool::end_zone(u32 index) {
    auto& zone = _zones[index];
    zone.contained_zones = u32(_zones.size()) - index - 1;
    zone.cpu_nanos = _chrono.elapsed().to_nanos() - zone.cpu_nanos;
    zone.end_query = alloc_query(PipelineStage::EndOfPipe);
}

}


