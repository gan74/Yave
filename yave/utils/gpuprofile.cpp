/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "gpuprofile.h"

#if 0

#include <yave/graphics/device/Queue.h>

#include <external/tracy/TracyVulkan.hpp>

namespace yave {

static tracy::VkCtx* profiling_ctx = nullptr;
static VkCommandPool profiling_cmd_pool = {};

static thread_local std::array<Uninitialized<tracy::VkCtxScope>, 64> gpu_scopes;
static thread_local usize gpu_scope_count = 0;

void init_gpu_profiling_ctx(const Queue& queue) {
    y_debug_assert(!profiling_ctx);

    VkCommandPoolCreateInfo create_info = vk_struct();
    {
        create_info.queueFamilyIndex = queue.family_index();
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    vk_check(vkCreateCommandPool(vk_device(), &create_info, vk_allocation_callbacks(), &profiling_cmd_pool));

    VkCommandBufferAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = profiling_cmd_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandBuffer buffer = {};
    vk_check(vkAllocateCommandBuffers(vk_device(), &allocate_info, &buffer));

    profiling_ctx = TracyVkContext(vk_physical_device(), vk_device(), queue.vk_queue(), buffer);
}

void deinit_gpu_profiling_ctx() {
    if(profiling_ctx) {
        TracyVkDestroy(profiling_ctx);
        vkDestroyCommandPool(vk_device(), profiling_cmd_pool, vk_allocation_callbacks());
        profiling_ctx = nullptr;
    }
}

void push_gpu_profiling_zone(void* cmd_buffer, std::string_view name, std::string_view file, std::string_view function, u32 line) {
    gpu_scopes[gpu_scope_count++].init(
            profiling_ctx,
            line,
            file.data(), file.size(),
            function.data(), function.size(),
            name.data(), name.size(),
            static_cast<VkCommandBuffer>(cmd_buffer), true);
}

void pop_gpu_profiling_zone() {
    gpu_scopes[--gpu_scope_count].destroy();
}

void collect_gpu_events(const Queue& queue) {
    if(profiling_ctx) {
        y_profile();

        VkCommandBufferAllocateInfo allocate_info = vk_struct();
        {
            allocate_info.commandBufferCount = 1;
            allocate_info.commandPool = profiling_cmd_pool;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }

        VkCommandBuffer buffer = {};
        vk_check(vkAllocateCommandBuffers(vk_device(), &allocate_info, &buffer));

        VkCommandBufferBeginInfo begin_info = vk_struct();
        vk_check(vkBeginCommandBuffer(buffer, &begin_info));

        TracyVkCollect(profiling_ctx, buffer);

        vk_check(vkEndCommandBuffer(buffer));

        VkSubmitInfo submit_info = vk_struct();
        {
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &buffer;
        }

        const auto lock = y_profile_unique_lock(queue.lock());
        vk_check(vkQueueSubmit(queue.vk_queue(), 1, &submit_info, {}));
    }
}

}

#endif
