/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDTIMESTAMPPOOL_H
#define YAVE_GRAPHICS_COMMANDS_CMDTIMESTAMPPOOL_H

#include <yave/graphics/graphics.h>

#include <yave/graphics/barriers/PipelineStage.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <y/core/Chrono.h>
#include <y/core/String.h>
#include <y/core/Vector.h>

namespace yave {

class CmdTimestampPool : NonMovable {
    struct Zone {
        core::String name;
        u32 contained_zones = 0;
        u64 cpu_nanos = 0;
        u32 start_query = u32(-1);
        u32 end_query = u32(-1);
    };

    public:
        struct TimedZone {
            std::string_view name;
            u32 contained_zones = 0;
            u64 cpu_nanos = 0;
            double gpu_nanos = 0.0;
        };


        CmdTimestampPool() = default;
        CmdTimestampPool(const CmdBufferRecorderBase& recorder);

        ~CmdTimestampPool();

        VkCommandBuffer vk_cmd_buffer() const;

        bool is_empty() const;
        bool is_ready(bool wait = false) const;


        template<typename F>
        bool for_each_zone(F&& func, bool wait = false) {
            if(!is_ready(wait)) {
                return false;
            }

            const double timestamp_to_ns = device_properties().timestamp_period;
            for(const Zone& zone : _zones) {
                const double gpu_nanos = (_results[zone.end_query] - _results[zone.start_query]) * timestamp_to_ns;
                const TimedZone tz{zone.name, zone.contained_zones, zone.cpu_nanos, gpu_nanos};
                func(tz);
            }

            return true;
        }


    private:
        friend class CmdBufferRegion;

        u32 begin_zone(core::String name);
        void end_zone(u32 index);

    private:
        u32 alloc_query(PipelineStage stage);

        core::SmallVector<VkHandle<VkQueryPool>, 2> _pools;
        VkCommandBuffer _cmd_buffer = {};
        u32 _query_index = 0;

        core::Chrono _chrono;
        core::Vector<Zone> _zones;

        core::Vector<u64> _results;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDTIMESTAMPPOOL_H

