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

#include "RendererSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/components/TransformableComponent.h>

namespace yave {

RendererSystem::TransformManager::TransformManager(ecs::EntityWorld& world) : _world(world) {
    _transform_destroyed = _world.on_destroyed<TransformableComponent>().subscribe([this](ecs::EntityId, TransformableComponent& tr) {
        free_index(tr);
    });
}

RendererSystem::TransformManager::~TransformManager() {
    auto query = _world.query<TransformableComponent>();
    for(const auto& [tr] : query.components()) {
        free_index(tr);
    }

    y_debug_assert(_free.size() == _transform_buffer.size());
}

RendererSystem::TransformManager::TransformBuffer RendererSystem::TransformManager::alloc_buffer(usize size) {
    y_debug_assert(size > _transform_buffer.size());

    _free.set_min_capacity(size);
    for(usize i = _transform_buffer.size(); i != size; ++i) {
        _free.push_back(u32(i));
    }

    return TransformBuffer(size);
}

void RendererSystem::TransformManager::free_index(const TransformableComponent& tr) {
    if(tr._transform_index != u32(-1)) {
        _free.push_back(tr._transform_index);
        tr._transform_index = u32(-1);
    }
}

bool RendererSystem::TransformManager::alloc_index(const TransformableComponent& tr) {
    if(tr._transform_index == u32(-1)) {
        y_debug_assert(!_free.is_empty());
        tr._transform_index = _free.pop();
        return true;
    }
    return false;
}

void RendererSystem::TransformManager::tick(bool only_recent) {
    y_profile();

    auto moved_query = only_recent ? _world.query<ecs::Changed<TransformableComponent>>() : _world.query<TransformableComponent>();

    {
        y_profile_zone("tracking stopped objects");
        for(const ecs::EntityId id : moved_query.ids()) {
            _last_moved.erase(id);
        }
    }

    const usize moved_count = moved_query.size();
    const usize stopped_count = _last_moved.size();
    if(const usize update_count = moved_count + stopped_count) {
        y_profile_msg(fmt_c_str("{} objects moved", moved_count));
        y_profile_zone("updating transform buffer");

        TransformBuffer new_buffer;
        auto realloc_if_needed = [&] {
            if(_free.is_empty()) {
                y_debug_assert(new_buffer.is_null());
                const usize new_base_size = _transform_buffer.size() + moved_count;
                new_buffer = alloc_buffer(2_uu << log2ui(new_base_size));
            }
        };

        auto transform_staging = TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::Staging>(update_count);
        auto index_staging = TypedBuffer<i32, BufferUsage::StorageBit, MemoryType::Staging>(update_count);

        u32 index = 0;
        {
            auto transform_mapping = transform_staging.map(MappingAccess::WriteOnly);
            auto index_mapping = index_staging.map(MappingAccess::WriteOnly);

            for(const auto& [tr] : moved_query.components()) {
                realloc_if_needed();

                const bool newly_allocated = alloc_index(tr);
                transform_mapping[index] = tr.transform();
                index_mapping[index] = newly_allocated ? -i32(tr._transform_index) : i32(tr._transform_index);
                ++index;
            }

            auto stopped_query = _world.query<TransformableComponent>(_last_moved.ids());
            for(const auto& [tr] : stopped_query.components()) {
                realloc_if_needed();

                y_debug_assert(tr._transform_index != u32(-1));
                transform_mapping[index] = tr.transform();
                index_mapping[index] = -i32(tr._transform_index);
                ++index;
            }
        }

        y_debug_assert(index <= update_count);

        {
            ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
            {
                if(!new_buffer.is_null()) {
                    if(!_transform_buffer.is_null()) {
                        recorder.unbarriered_copy(_transform_buffer, SubBuffer<BufferUsage::TransferDstBit>(new_buffer, _transform_buffer.byte_size(), 0));
                        recorder.barriers(BufferBarrier(new_buffer, PipelineStage::TransferBit, PipelineStage::ComputeBit));
                    }
                    std::swap(_transform_buffer, new_buffer);
                }

                const auto& program = device_resources()[DeviceResources::UpdateTransformsProgram];
                recorder.dispatch_size(program, math::Vec2ui(index, 1), DescriptorSet(_transform_buffer, transform_staging, index_staging, InlineDescriptor(index)));
            }
            recorder.submit_async();
        }
    }


    {
        y_profile_zone("gathering moved objets for next frame");
        _last_moved.make_empty();
        for(const ecs::EntityId id : moved_query.ids()) {
            _last_moved.insert(id);
        }
    }
}





RendererSystem::Renderer::~Renderer() {
}

const RendererSystem* RendererSystem::Renderer::parent() const {
    return _parent;
}

const ecs::EntityWorld& RendererSystem::Renderer::world() const {
    return _parent->world();
}


RendererSystem::RendererSystem() : ecs::System("RendererSystem") {
}

void RendererSystem::destroy() {
    _transform_manager = nullptr;
}

void RendererSystem::setup() {
    _transform_manager = std::make_unique<TransformManager>(world());
    _transform_manager->tick(false);
}

void RendererSystem::tick() {
    _transform_manager->tick(true);
}

RendererSystem::RenderFunc RendererSystem::prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType pass_type) const {
    if(ids.is_empty()) {
        return {};
    }

    auto funcs = core::Vector<typename Renderer::RenderFunc>::with_capacity(_renderers.size());

    for(const auto& renderer : _renderers) {
        funcs << renderer->prepare_render(builder, view, ids, pass_type);
    }

    return [render_funcs = std::move(funcs)](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        for(auto&& func : render_funcs) {
            if(func) {
                func(render_pass, pass);
            }
        }
    };
}

}


