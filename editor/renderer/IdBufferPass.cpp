/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "IdBufferPass.h"

#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/ecs/ecs.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/MeshData.h>


Y_TODO(merge with scene sub pass?)

// mostly copied from SceneRednerSubPass and EditorPass
namespace editor {

static usize render_world(RenderPassRecorder& recorder, const FrameGraphPass* pass,
                          const SceneView& scene_view,
                          const FrameGraphMutableTypedBufferId<Renderable::CameraData> camera_buffer,
                          const FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer,
                          const FrameGraphMutableTypedBufferId<u32> id_buffer,
                          usize index = 0) {
    y_profile();

    const ecs::EntityWorld& world = scene_view.world();

    auto camera_mapping = pass->resources().mapped_buffer(camera_buffer);
    camera_mapping[0] = scene_view.camera();

    auto transform_mapping = pass->resources().mapped_buffer(transform_buffer);
    const auto transforms = pass->resources().buffer<BufferUsage::AttributeBit>(transform_buffer);

    auto id_mapping = pass->resources().mapped_buffer(id_buffer);
    const auto ids = pass->resources().buffer<BufferUsage::AttributeBit>(id_buffer);

    recorder.bind_attrib_buffers({}, {transforms, ids});
    recorder.bind_material(resources()[EditorResources::IdMaterialTemplate], {pass->descriptor_sets()[0]});

    for(auto ent : world.view<TransformableComponent, StaticMeshComponent>()) {
        const auto& [tr, mesh] = ent.components();
        transform_mapping[index] = tr.transform();
        id_mapping[index] = ent.id().index();
        mesh.render_mesh(recorder, u32(index));
        ++index;
    }

    return index;
}

IdBufferPass IdBufferPass::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat id_format = VK_FORMAT_R32_UINT;

    FrameGraphPassBuilder builder = framegraph.add_pass("ID pass");

    auto camera_buffer = builder.declare_typed_buffer<Renderable::CameraData>();
    const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_batch_size);
    const auto id_buffer = builder.declare_typed_buffer<u32>(max_batch_size);

    const auto depth = builder.declare_image(depth_format, size);
    const auto id = builder.declare_image(id_format, size);

    IdBufferPass pass;
    pass.scene_view = view;
    pass.depth = depth;
    pass.id = id;

    builder.add_uniform_input(camera_buffer);
    builder.add_attrib_input(transform_buffer);
    builder.add_attrib_input(id_buffer);
    builder.map_update(transform_buffer);
    builder.map_update(id_buffer);

    builder.add_depth_output(depth);
    builder.add_color_output(id);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        render_world(render_pass, self, view, camera_buffer, transform_buffer, id_buffer);
    });

    return pass;
}

}

