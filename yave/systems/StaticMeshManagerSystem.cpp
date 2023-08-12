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

StaticMeshManagerSystem::RenderList StaticMeshManagerSystem::create_render_list(core::Span<ecs::EntityId> ids) const {
    auto query = world().query<StaticMeshComponent>(ids);
    if(query.is_empty()) {
        return {};
    }

    auto batches = core::vector_with_capacity<Batch>(query.size());
    for(const auto& [id, comp] : query.id_components()) {
        const auto [mesh] = comp;
        if(!mesh.mesh()) {
            continue;
        }

        y_debug_assert(mesh.has_instance_index());

        auto push_material = [](const AssetPtr<Material>& material, Batch& batch) {
            if(material) {
                batch.material_index = material->draw_data().index();
                batch.material_template = material->material_template();
                return true;
            }
            return false;
        };

        if(mesh.materials().size() == 1) {
            Batch batch;
            if(push_material(mesh.materials()[0], batch)) {
                batch.transform_index = mesh.instance_index();
                batch.draw_cmd = mesh.mesh()->draw_command();
                batch.id = id;
                batches << batch;
            }
        } else {
            for(usize i = 0; i != mesh.materials().size(); ++i) {
                Batch batch;
                if(push_material(mesh.materials()[i], batch)) {
                    batch.transform_index = mesh.instance_index();
                    batch.draw_cmd = mesh.mesh()->sub_meshes()[i];
                    batch.id = id;
                    batches << batch;
                }
            }
        }

    }

    RenderList list;
    list._parent = this;
    list._batches = std::move(batches);
    return list;
}


void StaticMeshManagerSystem::RenderList::draw(RenderPassRecorder& recorder) const {
    TypedBuffer<math::Vec2ui, BufferUsage::StorageBit, MemoryType::CpuVisible> indices(_batches.size());

    {
        auto mapping = indices.map(MappingAccess::WriteOnly);
        for(usize i = 0; i != _batches.size(); ++i) {
            mapping[i] = math::Vec2ui(_batches[i].transform_index, _batches[i].material_index);
        }
    }

    DescriptorSet set(std::array{
        Descriptor(_parent->transform_buffer()),
        Descriptor(indices),
        Descriptor(material_allocator().material_buffer()),
    });

    recorder.bind_mesh_buffers(mesh_allocator().mesh_buffers());

    const MaterialTemplate* previous = nullptr;
    for(usize i = 0; i != _batches.size(); ++i) {
        const auto& batch = _batches[i];
        y_debug_assert(batch.material_template);

        if(batch.material_template != previous) {
            previous = batch.material_template;
            std::array<DescriptorSetBase, 2> sets = {set, texture_library().descriptor_set()};
            recorder.bind_material_template(_batches[i].material_template, sets, true);
        }

        recorder.draw(batch.draw_cmd.vk_indirect_data(u32(i)));
    }
}

const StaticMeshManagerSystem* StaticMeshManagerSystem::RenderList::parent() const {
    return _parent;
}

core::Span<StaticMeshManagerSystem::Batch> StaticMeshManagerSystem::RenderList::batches() const {
    return _batches;
}

}


