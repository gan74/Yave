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

#include "TimeQueryPool.h"


namespace yave {

TimeQueryPoolData::TimeQueryPoolData(VkCommandBuffer cmd_buffer) : _cmd_buffer(cmd_buffer) {
    y_debug_assert(_cmd_buffer);
}

TimeQueryPoolData::~TimeQueryPoolData() {
    for(auto& pool : _pools) {
        const bool is_last = pool == _pools.last();
        u32 data[pool_size] = {};
        vk_check(vkGetQueryPoolResults(vk_device(), pool, 0, is_last ? _next_query : pool_size, sizeof(data), data, sizeof(data[0]), VK_QUERY_RESULT_WAIT_BIT));

        destroy_graphic_resource(std::move(pool));
    }
}

void TimeQueryPoolData::alloc_pool() {
    VkQueryPoolCreateInfo create_info = vk_struct();
    {
        create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        create_info.queryCount = pool_size;
    }

    auto& pool = _pools.emplace_back();
    vk_check(vkCreateQueryPool(vk_device(), &create_info, vk_allocation_callbacks(), pool.get_ptr_for_init()));
    vkCmdResetQueryPool(_cmd_buffer, pool, 0, pool_size);
    _next_query = 0;
}

std::pair<u32, u32> TimeQueryPoolData::alloc_query() {
    if(_next_query == pool_size) {
        alloc_pool();
    }

    return {u32(_pools.size() - 1), _next_query++};
}




TimeQuery::TimeQuery(const std::shared_ptr<TimeQueryPoolData>& data, u32 pool, u32 query) :
    _data(data), _pool(pool), _query(query) {
}

bool TimeQuery::is_null() const {
    return !_data && !_has_result;
}

bool TimeQuery::is_ready() const {
    if(_has_result) {
        return true;
    }

    const VkResult result = vkGetQueryPoolResults(vk_device(), _data->_pools[_pool], _query, 1, sizeof(u64), &_result, sizeof(u64), VK_QUERY_RESULT_64_BIT);
    vk_check(result);

    if(result == VK_SUCCESS) {
        _has_result = true;
        _data = nullptr;
    }

    return _has_result;
}

u64 TimeQuery::get() const {
    if(_has_result) {
        return _result;
    }

    y_debug_assert(_data);
    vk_check(vkGetQueryPoolResults(vk_device(), _data->_pools[_pool], _query, 1, sizeof(u64), &_result, sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    _has_result = true;
    _data = nullptr;

    return _result;
}


TimeQueryPool::TimeQueryPool(VkCommandBuffer cmd_buffer) : _data(std::make_shared<TimeQueryPoolData>(cmd_buffer)) {
}

VkCommandBuffer TimeQueryPool::vk_cmd_buffer() const {
    return _data->_cmd_buffer;
}

TimeQuery TimeQueryPool::query(PipelineStage stage) {
    const auto [pool, query] = _data->alloc_query();
    vkCmdWriteTimestamp(_data->_cmd_buffer, VkPipelineStageFlagBits(stage), _data->_pools[pool], query);
    return TimeQuery(_data, pool, query);
}


}


