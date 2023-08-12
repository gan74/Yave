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

#include "StaticMeshManagerSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

namespace yave {


StaticMeshManagerSystem::StaticMeshManagerSystem() : ecs::System("StaticMeshManagerSystem"), _transforms(max_transforms) {
    _free.set_min_size(_transforms.size());
    std::iota(_free.begin(), _free.end(), 0);
}

void StaticMeshManagerSystem::destroy() {
    auto query = world().query<StaticMeshComponent>();
    for(auto&& [mesh] : query.components()) {
        if(mesh.has_instance_index()) {
            _free.push_back(mesh._instance_index);
            mesh._instance_index = u32(-1);
        }
    }

    y_debug_assert(_free.size() == _transforms.size());
}

void StaticMeshManagerSystem::setup() {
    run_tick(false);
}

void StaticMeshManagerSystem::tick() {
    run_tick(true);
}

void StaticMeshManagerSystem::run_tick(bool only_recent) {
    auto moved_query = [&](auto query) {
        if(!query.size()) {
            return;
        }

        auto mapping = _transforms.map(MappingAccess::ReadWrite);
        for(const auto& [mesh, tr] : query.components()) {
            if(!mesh.has_instance_index()) {
                y_always_assert(!_free.is_empty(), "Max number of transforms reached");
                mesh._instance_index = _free.pop();
            }

            y_debug_assert(mesh.has_instance_index());
            mapping[mesh._instance_index] = tr.transform();
        }
    };

    if(only_recent) {
        moved_query(world().query<StaticMeshComponent, ecs::Changed<TransformableComponent>>());
    } else {
        moved_query(world().query<StaticMeshComponent, TransformableComponent>());
    }

    auto removed = world().query<ecs::Removed<StaticMeshComponent>>();
    for(const auto& [mesh] : removed.components()) {
        if(mesh.has_instance_index()) {
            _free.push_back(mesh._instance_index);
            mesh._instance_index = u32(-1);
        }
    }
}

void StaticMeshManagerSystem::render(RenderPassRecorder& recorder, TypedSubBuffer<uniform::Camera, BufferUsage::UniformBit> camera, core::Span<ecs::EntityId> ids) const {
    auto query = world().query<StaticMeshComponent>(ids);
    if(query.is_empty()) {
        return;
    }

    auto mesh_indices = core::vector_with_capacity<math::Vec2ui>(query.size() * 8);
    auto material_templates = core::vector_with_capacity<const MaterialTemplate*>(query.size() * 8);
    auto mesh_draw_cmds = core::vector_with_capacity<MeshDrawCommand>(query.size() * 8);

    for(const auto& [mesh] : query.components()) {
        if(!mesh.mesh()) {
            continue;
        }

        y_debug_assert(mesh.has_instance_index());

        auto push_material = [&](const StaticMeshComponent& mesh, const AssetPtr<Material>& material) {
            if(material) {
                mesh_indices.emplace_back(mesh.instance_index(), material->draw_data().index());
                material_templates.emplace_back(material->material_template());
                return true;
            }
            return false;
        };

        if(mesh.materials().size() == 1) {
            if(push_material(mesh, mesh.materials()[0])) {
                mesh_draw_cmds.emplace_back(mesh.mesh()->draw_command());
            }
        } else {
            for(usize i = 0; i != mesh.materials().size(); ++i) {
                if(push_material(mesh, mesh.materials()[i])) {
                    mesh_draw_cmds.emplace_back(mesh.mesh()->sub_meshes()[i]);
                }
            }
        }
    }

    y_debug_assert(mesh_draw_cmds.size() == mesh_indices.size());
    y_debug_assert(mesh_draw_cmds.size() == material_templates.size());

    if(mesh_draw_cmds.is_empty()) {
        return;
    }

    TypedBuffer<math::Vec2ui, BufferUsage::StorageBit | BufferUsage::TransferDstBit> indices(mesh_indices.size());
    {
        CmdBufferRecorder staging_recorder = create_disposable_cmd_buffer();
        BufferMappingBase::stage(staging_recorder, indices, mesh_indices.data());
        command_queue().submit(std::move(staging_recorder));
    }

    DescriptorSet set(std::array{
        Descriptor(camera),
        Descriptor(_transforms),
        Descriptor(indices),
        Descriptor(material_allocator().material_buffer()),
    });

    recorder.bind_mesh_buffers(mesh_allocator().mesh_buffers());
    for(usize i = 0; i != mesh_draw_cmds.size(); ++i) {
        if(!i || material_templates[i] != material_templates[i - 1]) {
            std::array<DescriptorSetBase, 2> sets = {set, texture_library().descriptor_set()};
            recorder.bind_material_template(material_templates[i], sets);
        }
        recorder.draw(mesh_draw_cmds[i].vk_indirect_data(u32(i)));
    }
}

}


