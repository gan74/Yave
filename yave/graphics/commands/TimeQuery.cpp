/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "TimeQuery.h"

#include <yave/device/DeviceUtils.h>

namespace yave {

static VkQueryPool create_pool(DevicePtr dptr) {
    VkQueryPoolCreateInfo create_info = vk_struct();
    {
        create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        create_info.queryCount = 2;
    }

    VkQueryPool pool = {};
    vk_check(vkCreateQueryPool(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &pool));
    return pool;
}

TimeQuery::TimeQuery(DevicePtr dptr)  : DeviceLinked(dptr), _pool(create_pool(dptr)) {
}

TimeQuery::~TimeQuery() {
    destroy(_pool);
}

void TimeQuery::start(CmdBufferRecorder& recorder) {
    vkCmdWriteTimestamp(recorder.vk_cmd_buffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _pool, 0);
}

void TimeQuery::stop(CmdBufferRecorder& recorder) {
    vkCmdWriteTimestamp(recorder.vk_cmd_buffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _pool, 1);
}

core::Duration TimeQuery::get() {
    std::array<u64, 2> results;
    vk_check(vkGetQueryPoolResults(vk_device(device()), _pool, 0, 2, 2 * sizeof(u64), results.data(), 0, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    return core::Duration::nanoseconds(results[1] - results[0]);
}

}

