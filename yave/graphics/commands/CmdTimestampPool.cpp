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

static constexpr u32 timestamp_pool_size = 256;

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
        _cmd_buffer(recorder.vk_cmd_buffer()) {
}

CmdTimestampPool::~CmdTimestampPool() {
    for(auto& pool : _pools) {
        destroy_graphic_resource(std::move(pool));
    }
}

VkCommandBuffer CmdTimestampPool::vk_cmd_buffer() const {
    return _cmd_buffer;
}

bool CmdTimestampPool::is_empty() const {
    return !_query_index;
}

bool CmdTimestampPool::is_ready(bool wait) const {
    const VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : 0);
    for(usize i = 0; i != _pools.size(); ++i) {
        const usize pool_index = _pools.size() - i - 1;
        const VkQueryPool pool = _pools[pool_index];
        const u32 count = i ? timestamp_pool_size : _query_index % timestamp_pool_size;
        void* result_ptr = const_cast<u64*>(&_results[pool_index * timestamp_pool_size]);
        y_debug_assert(count);
        if(VK_NOT_READY == vk_check(vkGetQueryPoolResults(vk_device(), pool, 0, count, sizeof(u64) * count, result_ptr, sizeof(u64), flags))) {
            return false;
        }
    }
    return true;
}

u32 CmdTimestampPool::alloc_query(PipelineStage stage) {
    if(_query_index % timestamp_pool_size == 0) {
        _pools.emplace_back(create_pool(_cmd_buffer));
        _results.set_min_size(_pools.size() * timestamp_pool_size);
    }

    const u32 index = _query_index++;
    vkCmdWriteTimestamp(_cmd_buffer, VkPipelineStageFlagBits(stage), _pools.last(), index % timestamp_pool_size);
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


