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

#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

namespace yave {

StaticMeshRendererSystem::StaticMeshRendererSystem() : ecs::System("StaticMeshRendererSystem") {
    _transforms = alloc_buffer(default_buffer_size);
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
    _mesh_destroyed = world().on_destroyed<StaticMeshComponent>().subscribe([this](ecs::EntityId, StaticMeshComponent& mesh) {
        free_index(mesh._transform_index);
        free_index(mesh._last_transform_index);
    });

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

        TransformBuffer new_buffer;
        auto realloc_if_needed = [&] {
            if(_free.is_empty()) {
                y_debug_assert(new_buffer.is_null());
                const usize new_base_size = _transforms.size() + moved_count;
                new_buffer = alloc_buffer(2_uu << log2ui(new_base_size));
            }
        };

        auto transform_staging = TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::Staging>(moved_count);
        auto index_staging = TypedBuffer<u32, BufferUsage::StorageBit, MemoryType::Staging>(moved_count);

        u32 index = 0;
        {
            auto transform_mapping = transform_staging.map(MappingAccess::WriteOnly);
            auto index_mapping = index_staging.map(MappingAccess::WriteOnly);
            for(const auto& [mesh, tr] : query.components()) {
                realloc_if_needed();

                std::swap(mesh._last_transform_index, mesh._transform_index);
                alloc_index(mesh._transform_index);

                y_debug_assert(mesh.has_transform_index());

                transform_mapping[index] = tr.transform();
                index_mapping[index] = mesh._transform_index;
                ++index;
            }
        }

        {
            ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
            {
                if(!new_buffer.is_null()) {
                    recorder.unbarriered_copy(_transforms, SubBuffer<BufferUsage::TransferDstBit>(new_buffer, _transforms.byte_size(), 0));
                    recorder.barriers(BufferBarrier(new_buffer, PipelineStage::TransferBit, PipelineStage::ComputeBit));
                    std::swap(_transforms, new_buffer);
                }

                const auto& program = device_resources()[DeviceResources::UpdateTransformsProgram];
                recorder.dispatch_size(program, math::Vec2ui(index, 1), DescriptorSet(_transforms, transform_staging, index_staging, InlineDescriptor(index)));
            }
            recorder.submit_async();
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
}

StaticMeshRendererSystem::TransformBuffer StaticMeshRendererSystem::alloc_buffer(usize size) {
    y_debug_assert(size > _transforms.size());

    _free.set_min_capacity(size);
    for(usize i = _transforms.size(); i != size; ++i) {
        _free.push_back(u32(i));
    }

    return TransformBuffer(size);
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


