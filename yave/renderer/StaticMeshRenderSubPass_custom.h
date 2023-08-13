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
#ifndef YAVE_RENDERER_STATICMESHRENDERSUBPASS_CUSTOM_H
#define YAVE_RENDERER_STATICMESHRENDERSUBPASS_CUSTOM_H

#include "StaticMeshRenderSubPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/systems/StaticMeshRendererSystem.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>
#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>


namespace yave {

template<typename RenderFunc>
void StaticMeshRenderSubPass::render_custom(RenderPassRecorder& render_pass, const FrameGraphPass* pass, RenderFunc&& render_func) const {
    y_profile();

    if(!scene_view.has_world()) {
        return;
    }

    const ecs::EntityWorld& world = scene_view.world();
    auto query = world.query<StaticMeshComponent>(ids, tags);
    if(query.is_empty()) {
        return;
    }

    render_pass.bind_mesh_buffers(mesh_allocator().mesh_buffers());

    u32 index = 0;
    auto indices_mapping = pass->resources().map_buffer(indices_buffer);
    for(const auto& [id, comp] : query.id_components()) {
        const auto& [mesh] = comp;
        if(!mesh.mesh() || !mesh.has_instance_index()) {
            continue;
        }

        const u32 transform_index = mesh.instance_index();

        const auto materials = mesh.materials();
        if(materials.size() == 1) {
            if(const Material* mat = materials[0].get()) {
                render_func(id, mesh, mesh.mesh()->draw_command(), mat, index);
                indices_mapping[index++] = math::Vec2ui(transform_index, mat->draw_data().index());
            }
        } else {
            for(usize i = 0; i != materials.size(); ++i) {
                if(const Material* mat = materials[i].get()) {
                    render_func(id, mesh, mesh.mesh()->sub_meshes()[i], mat, index);
                    indices_mapping[index++] = math::Vec2ui(transform_index, mat->draw_data().index());
                }
            }
        }
    }
}

}

#endif // YAVE_RENDERER_STATICMESHRENDERSUBPASS_CUSTOM_H

