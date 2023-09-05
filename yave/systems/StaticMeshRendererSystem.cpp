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

#include "StaticMeshRendererSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

namespace yave {


StaticMeshRendererSystem::StaticMeshRendererSystem() : ecs::System("StaticMeshRendererSystem"), _transforms(max_transforms) {
    _free.set_min_size(_transforms.size());
    std::iota(_free.begin(), _free.end(), 0);
}

void StaticMeshRendererSystem::destroy() {
    auto query = world().query<StaticMeshComponent>();
    for(auto&& [mesh] : query.components()) {
        free_index(mesh._transform_index);
        free_index(mesh._last_transform_index);
    }

    y_debug_assert(_free.size() == _transforms.size());
}

void StaticMeshRendererSystem::setup() {
    run_tick(false);
}

void StaticMeshRendererSystem::tick() {
    run_tick(true);
}

void StaticMeshRendererSystem::run_tick(bool only_recent) {
    auto moved_query = [&](auto query) {
        const usize moved_count = query.size();

        if(!moved_count) {
            return;
        }

        y_profile_msg(fmt_c_str("{} objects moved", moved_count));

        _moved.make_empty();
        for(ecs::EntityId id : query.ids()) {
            _moved.insert(id);
            if(_prev_moved.contains(id)) {
                _prev_moved.erase(id);
            }
        }

        auto transform_staging = TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::CpuVisible>(moved_count);
        auto index_staging = TypedBuffer<u32, BufferUsage::StorageBit, MemoryType::CpuVisible>(moved_count);

        u32 index = 0;
        {
            auto transform_mapping = transform_staging.map(MappingAccess::WriteOnly);
            auto index_mapping = index_staging.map(MappingAccess::WriteOnly);
            for(const auto& [mesh, tr] : query.components()) {
                std::swap(mesh._last_transform_index, mesh._transform_index);
                alloc_index(mesh._transform_index);

                y_debug_assert(mesh.has_transform_index());

                transform_mapping[index] = tr.transform();
                index_mapping[index] = mesh._transform_index;
                ++index;
            }
        }

        {
            CmdBufferRecorder recorder = create_disposable_cmd_buffer();

            const DescriptorSet ds(std::array{
                Descriptor(_transforms),
                Descriptor(transform_staging),
                Descriptor(index_staging),
                Descriptor(InlineDescriptor(index))
            });

            const auto& program = device_resources()[DeviceResources::UpdateTransformsProgram];
            recorder.dispatch_size(program, math::Vec2ui(index, 1), ds);

            command_queue().submit(std::move(recorder));
        }
    };

    if(only_recent) {
        moved_query(world().query<StaticMeshComponent, ecs::Changed<TransformableComponent>>());
    } else {
        moved_query(world().query<StaticMeshComponent, TransformableComponent>());
    }


    auto previously_moved = world().query<StaticMeshComponent>(_prev_moved.ids());
    for(const auto& [mesh] : previously_moved.components()) {
        free_index(mesh._last_transform_index);
    }
    std::swap(_prev_moved, _moved);

    auto removed = world().query<ecs::Removed<StaticMeshComponent>>();
    for(const auto& [mesh] : removed.components()) {
        free_index(mesh._transform_index);
        free_index(mesh._last_transform_index);
    }
}

void StaticMeshRendererSystem::free_index(u32& index) {
    if(index != u32(-1)) {
        _free.push_back(index);
        index = u32(-1);
    }
}

void StaticMeshRendererSystem::alloc_index(u32& index) {
    if(index == u32(-1)) {
        y_always_assert(!_free.is_empty(), "Max number of transforms reached");
        index = _free.pop();
    }
}

}


