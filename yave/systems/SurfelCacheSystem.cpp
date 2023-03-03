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

#include "SurfelCacheSystem.h"

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/graphics.h>

#include <y/utils/log.h>

namespace yave {

static constexpr u32 max_instances = 4096;
static constexpr u32 max_surfels = max_instances * 4096;

SurfelCacheSystem::SurfelCacheSystem() : ecs::System("SurfelCacheSystem"), _surfels(max_surfels), _instances(max_instances) {
}

u32 SurfelCacheSystem::surfel_count() const {
    return _allocated_surfels;
}

u32 SurfelCacheSystem::instance_count() const {
    return _allocated_instances;
}

const TypedBuffer<Surfel, BufferUsage::StorageBit>& SurfelCacheSystem::surfel_buffer() const {
    return _surfels;
}

const TypedBuffer<SurfelCacheSystem::InstanceData, BufferUsage::StorageBit>& SurfelCacheSystem::instance_buffer() const {
    return _instances;
}

void SurfelCacheSystem::tick(ecs::EntityWorld& world) {
    auto query = world.query<ecs::Changed<TransformableComponent>, StaticMeshComponent>();

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("Update surfel cache");

        const ComputeProgram& program = device_resources().from_file("update_surfels.comp.spv");

        for(auto&& [tr, m] : query.components()) {
            if(!m.mesh()) {
                continue;
            }

            const StaticMesh& mesh = *m.mesh();
            const u32 surfel_count = u32(mesh._surfels.size());

            if(m._surfels_offset == u32(-1)) {

                if(_allocated_surfels + surfel_count > max_surfels || _allocated_instances + 1 > max_instances) {
                    log_msg("Surfel cache full", Log::Warning);
                    continue;
                }

                m._surfels_offset = _allocated_surfels;
                m._instance_index = _allocated_instances;

                _allocated_surfels += surfel_count;
                _allocated_instances += 1;
            }

            const AABB aabb = tr.to_global(m.aabb());
            const float scale = tr.transform().scale().max_component();

            const struct Params {
                InstanceData instance;
                u32 instance_index;
            } params = {
                {
                    tr.transform(),
                    aabb.center(),
                    aabb.radius(),
                    scale,
                    mesh._total_area / surfel_count,
                    surfel_count,
                    m._surfels_offset,
                },
                m._instance_index,
            };

            const std::array<Descriptor, 4> descriptors = {
                Descriptor(InlineDescriptor(params)),
                Descriptor(mesh._surfel_buffer),
                Descriptor(_surfels),
                Descriptor(_instances),
            };

            recorder.dispatch_size(program, math::Vec2ui(surfel_count, 1), DescriptorSet(descriptors));
        }
    }

    command_queue().submit(std::move(recorder));
}

}

