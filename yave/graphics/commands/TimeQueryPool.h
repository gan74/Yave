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
#ifndef YAVE_GRAPHICS_COMMANDS_TIMEQUERYPOOL_H
#define YAVE_GRAPHICS_COMMANDS_TIMEQUERYPOOL_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/barriers/PipelineStage.h>

#include <y/core/Vector.h>

#include <memory>

namespace yave {

class TimeQueryPoolData : NonMovable {
    static constexpr u32 pool_size = 64;

    public:
        TimeQueryPoolData(VkCommandBuffer cmd_buffer);
        ~TimeQueryPoolData();

    private:
        friend class TimeQueryPool;
        friend class TimestampQuery;

        std::pair<u32, u32> alloc_query();
        void alloc_pool();

        u32 _next_query = pool_size;
        core::Vector<VkHandle<VkQueryPool>> _pools;
        VkCommandBuffer _cmd_buffer = {};

};

class TimestampQuery {
    public:
        TimestampQuery() = default;

        bool is_null() const;

        bool is_ready() const;

        u64 get_ticks() const; // Needs to be multiplied by device_properties().timestamp_period !!!

    private:
        friend class TimeQueryPool;

        TimestampQuery(const std::shared_ptr<TimeQueryPoolData>& data, u32 pool, u32 query);

        mutable u64 _result = 0;
        mutable bool _has_result = false;
        mutable std::shared_ptr<TimeQueryPoolData> _data;

        u32 _pool = 0;
        u32 _query = 0;
};

class TimeQueryPool : NonMovable {
    public:
        TimeQueryPool(VkCommandBuffer cmd_buffer);

        TimestampQuery query(PipelineStage stage);

        VkCommandBuffer vk_cmd_buffer() const;

    private:
        std::shared_ptr<TimeQueryPoolData> _data;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_TIMEQUERYPOOL_H

