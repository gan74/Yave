/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "ScenePickingPass.h"

#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/entities/entities.h>

#include <editor/context/EditorContext.h>

Y_TODO(merge with scene sub pass?)

// mostly copied from SceneRednerSubPass and EditorEntityPass
namespace editor {

static usize render_world(ContextPtr ctx,
						  RenderPassRecorder& recorder, const FrameGraphPass* pass,
						  const SceneView& scene_view,
						  const FrameGraphMutableTypedBufferId<Renderable::CameraData> camera_buffer,
						  const FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer,
						  const FrameGraphMutableTypedBufferId<u32> id_buffer,
						  usize index = 0) {
	y_profile();

	const ecs::EntityWorld& world = scene_view.world();

	auto camera_mapping = pass->resources().mapped_buffer(camera_buffer);
	camera_mapping[0] = scene_view.camera().viewproj_matrix();

	auto transform_mapping = pass->resources().mapped_buffer(transform_buffer);
	const auto transforms = pass->resources().buffer<BufferUsage::AttributeBit>(transform_buffer);

	auto id_mapping = pass->resources().mapped_buffer(id_buffer);
	const auto ids = pass->resources().buffer<BufferUsage::AttributeBit>(id_buffer);

	recorder.bind_attrib_buffers({}, {transforms, ids});
	recorder.bind_material(ctx->resources()[EditorResources::PickingMaterialTemplate], {pass->descriptor_sets()[0]});

	for(auto ent : world.view(StaticMeshArchetype())) {
		const auto& [tr, mesh] = ent.components();
		transform_mapping[index] = tr.transform();
		id_mapping[index] = ent.index();
		mesh.render_mesh(recorder, u32(index));
		++index;
	}

	return index;
}

ScenePickingPass ScenePickingPass::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size) {
	static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
	static constexpr vk::Format id_format = vk::Format::eR32Uint;

	FrameGraphPassBuilder builder = framegraph.add_pass("Picking pass");

	auto camera_buffer = builder.declare_typed_buffer<Renderable::CameraData>();
	const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_batch_size);
	const auto id_buffer = builder.declare_typed_buffer<u32>(max_batch_size);

	const auto depth = builder.declare_image(depth_format, size);
	const auto id = builder.declare_image(id_format, size);

	ScenePickingPass pass;
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
			render_world(ctx, render_pass, self, view, camera_buffer, transform_buffer, id_buffer);
		});

	return pass;
}

}
