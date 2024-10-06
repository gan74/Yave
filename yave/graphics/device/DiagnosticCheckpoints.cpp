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

#include "DiagnosticCheckpoints.h"

#include <y/core/FixedArray.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


#include <vulkan/vk_enum_string_helper.h>

namespace yave {

const char* DiagnosticCheckpoints::extension_name() {
    return VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME;
}

DiagnosticCheckpoints::DiagnosticCheckpoints() : _checkpoints(1024) {
    y_always_assert(vkCmdSetCheckpointNV, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME " not loaded");
    y_always_assert(vkGetQueueCheckpointDataNV, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME " not loaded");
}


void DiagnosticCheckpoints::register_queue(VkQueue queue) {
    _queues << queue;
}

void DiagnosticCheckpoints::dump_checkpoints() const {
    log_msg("Dumping " VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME " checkpoints:", Log::Error);
    for(const VkQueue queue : _queues) {
        u32 count = 0;
        vkGetQueueCheckpointDataNV(queue, &count, nullptr);

        core::Vector<VkCheckpointDataNV> datas(count, vk_struct());
        vkGetQueueCheckpointDataNV(queue, &count, datas.data());

        log_msg(fmt("  {} last checkpoints executed in queue:", count), Log::Error);
        for(const VkCheckpointDataNV& data : datas) {
            const usize index = std::bit_cast<usize>(data.pCheckpointMarker);
            const auto& ck = _checkpoints[index % _checkpoints.size()];
            if(ck.first != index) {
                log_msg(fmt("    {}: ???", string_VkPipelineStageFlagBits(data.stage)), Log::Error);
            } else {
                log_msg(fmt("    {}: {}", string_VkPipelineStageFlagBits(data.stage), ck.second), Log::Error);
            }
        }
    }
}

void DiagnosticCheckpoints::set_checkpoint(VkCommandBuffer cmd_buffer, const char* data) const {
    const usize index = ++_count;
    auto& ck = _checkpoints[index % _checkpoints.size()];
    {
        ck.first = index;
        ck.second = data;
    }

    vkCmdSetCheckpointNV(cmd_buffer, std::bit_cast<const void*>(index));
}

DiagnosticCheckpoints::~DiagnosticCheckpoints() {
}

}

