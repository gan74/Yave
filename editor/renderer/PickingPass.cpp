/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "PickingPass.h"

#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/entities/entities.h>

#include <editor/context/EditorContext.h>

Y_TODO(merge with scene sub pass?)

// mostly copied from SceneRednerSubPass
namespace editor {

static constexpr usize max_batch_size = 128 * 1024;

static usize render_world(ContextPtr ctx, RenderPassRecorder& recorder, const PickingPass& picking_pass, const FrameGraphPass* pass, usize index = 0) {
	y_profile();

	const ecs::EntityWorld& world = picking_pass.scene_view.world();

	auto transform_mapping = pass->resources()->mapped_buffer(picking_pass.transform_buffer);
	auto transforms = pass->resources()->buffer<BufferUsage::AttributeBit>(picking_pass.transform_buffer);

	recorder.bind_attrib_buffers({transforms, transforms});
	recorder.bind_material(ctx->resources()[EditorResources::PickingMaterialTemplate], {pass->descriptor_sets()[0]});

	for(const auto& [tr, me] : world.view(StaticMeshArchetype())) {
		transform_mapping[index] = tr.transform();
		me.render_mesh(recorder, u32(index));
		++index;
	}

	return index;
}

PickingPass PickingPass::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size) {
	unused(ctx);

	static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
	static constexpr vk::Format id_format = vk::Format::eR32Uint;

	FrameGraphPassBuilder builder = framegraph.add_pass("Picking pass");

	auto camera_buffer = builder.declare_typed_buffer<Renderable::CameraData>();
	auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_batch_size);
	auto depth = builder.declare_image(depth_format, size);
	auto id = builder.declare_image(id_format, size);

	PickingPass pass;
	pass.scene_view = view;
	pass.camera_buffer = camera_buffer;
	pass.transform_buffer = transform_buffer;
	pass.depth = depth;
	pass.id = id;

	builder.add_uniform_input(camera_buffer);
	builder.add_attrib_input(transform_buffer);
	builder.map_update(camera_buffer);
	builder.map_update(transform_buffer);

	builder.add_depth_output(depth);
	builder.add_color_output(id);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			render_world(ctx, render_pass, pass, self);
		});

	return pass;
}

}
