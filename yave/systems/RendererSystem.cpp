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
#include <yave/systems/TransformableManagerSystem.h>

#include <yave/ecs/SparseComponentSet.h>

namespace yave {

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
    _transform_buffer = {};
}

void RendererSystem::setup() {
    update_transform_buffer(false);
}

void RendererSystem::tick() {
    update_transform_buffer(true);
}

void RendererSystem::update_transform_buffer(bool only_recent) {
    y_profile();

    const TransformableManagerSystem* transformable_manager = world().find_system<TransformableManagerSystem>();

    TransformBuffer new_buffer;
    if(transformable_manager->transform_count() > _transform_buffer.size()) {
        const usize new_base_size = 2_uu << log2ui(transformable_manager->transform_count());
        y_debug_assert(new_base_size > _transform_buffer.size());
        new_buffer = TransformBuffer(new_base_size);
    };


    const usize update_count = only_recent
        ? (transformable_manager->moved().size() + transformable_manager->stopped().size())
        : world().component_set<TransformableComponent>().size();

    y_debug_assert(new_buffer.is_null() || update_count);


    if(!update_count) {
        return;
    }



    auto transform_staging = TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::Staging>(update_count);
    auto index_staging = TypedBuffer<u32, BufferUsage::StorageBit, MemoryType::Staging>(update_count);

    u32 updates = 0;
    {
        auto transform_mapping = transform_staging.map(MappingAccess::WriteOnly);
        auto index_mapping = index_staging.map(MappingAccess::WriteOnly);

        const auto& transformables = world().component_set<TransformableComponent>();

        if(only_recent) {
            for(const auto& [id, index] : transformable_manager->moved()) {
                transform_mapping[updates] = transformables[id].transform();
                index_mapping[updates] = (index.index | (index.reset_transform ? 0x80000000 : 0x00000000)); // Noop on little endian
                ++updates;
            }
            for(const auto& [id, index] : transformable_manager->stopped()) {
                transform_mapping[updates] = transformables[id].transform();
                index_mapping[updates] = index.index | 0x80000000;
                ++updates;
            }
        } else {
            for(const auto& [id, tr] : transformables) {
                transform_mapping[updates] = tr.transform();
                index_mapping[updates] = tr.transform_index() | 0x80000000;
                ++updates;
            }
        }
    }


    {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        {
            const auto region = recorder.region("Transform update");
            if(!new_buffer.is_null()) {
                if(!_transform_buffer.is_null()) {
                    recorder.unbarriered_copy(_transform_buffer, SubBuffer<BufferUsage::TransferDstBit>(new_buffer, _transform_buffer.byte_size(), 0));
                    recorder.barriers(BufferBarrier(new_buffer, PipelineStage::TransferBit, PipelineStage::ComputeBit));
                }
                std::swap(_transform_buffer, new_buffer);
            }

            const auto& program = device_resources()[DeviceResources::UpdateTransformsProgram];
            recorder.dispatch_size(program, math::Vec2ui(updates, 1), DescriptorSet(_transform_buffer, transform_staging, index_staging, InlineDescriptor(updates)));
        }
        recorder.submit_async();
    }
}

RendererSystem::RenderFunc RendererSystem::prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType pass_type) const {
    y_profile();

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


