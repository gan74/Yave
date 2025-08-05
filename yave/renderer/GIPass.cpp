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

#include "GIPass.h"

#include <yave/renderer/CameraBufferPass.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/scene/SceneView.h>

#include <yave/graphics/shader_structs.h>

namespace yave {

GIPass GIPass::create(FrameGraph& framegraph, const CameraBufferPass& camera) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("GI pass");

    const math::Vec2ui size(128, 128);

    const SceneView& scene_view = camera.view;
    const Scene* scene = scene_view.scene();
    const usize mesh_count = scene->meshes().size();

    const auto probe = builder.declare_image(VK_FORMAT_R32_UINT, size);
    const auto meshes = builder.declare_typed_buffer<shader::GIMeshInfo>(mesh_count);

    const MeshDrawBuffers& mesh_buffers = mesh_allocator().mesh_buffers();
    const StorageSubBuffer position_buffer = mesh_buffers.attrib_buffers()[usize(VertexStreamType::Position)];

    builder.clear_before_pass(probe);

    builder.map_buffer(meshes);

    builder.add_storage_output(probe);
    builder.add_uniform_input(camera.camera);
    builder.add_storage_input(meshes);
    builder.add_external_input(Descriptor(position_buffer));
    builder.add_external_input(Descriptor(scene->transform_manager().transform_buffer()));
    builder.add_inline_input(u32(mesh_count));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        {
            usize index = 0;
            auto mapping = self->resources().map_buffer(meshes);
            for(const auto& mesh : scene->meshes()) {
                const AssetPtr<StaticMesh>& mesh_component = mesh.component.mesh();
                if(mesh_component) {
                    const MeshDrawData& draw_data = mesh_component->draw_data();
                    mapping[index++] = shader::GIMeshInfo {
                        draw_data.vertex_count(),
                        u32(draw_data.draw_command().vertex_offset),
                        mesh.transform_index,
                        0
                    };
                } else {
                    mapping[index++] = {};
                }
            }

            y_debug_assert(index == mesh_count);
        }

        const auto& program = device_resources()[DeviceResources::GIProgram];
        recorder.dispatch_threads(program, math::Vec3ui(128, mesh_count, 1), self->descriptor_set());
    });

    GIPass pass;
    pass.probe = probe;
    return pass;
}

}

