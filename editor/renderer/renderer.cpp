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

#include "renderer.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPassBuilder.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/entities/entities.h>

#include <editor/context/EditorContext.h>

// mostly copied from SceneRednerSubPass
namespace editor {

static constexpr usize max_batch_size = 128 * 1024;

static usize render(ContextPtr ctx,
					RenderPassRecorder& recorder, const FrameGraphPass* pass,
					const SceneView& scene_view,
					FrameGraphMutableTypedBufferId<math::Matrix4<>> camera_buffer,
					FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer) {
	y_profile();

	{
		auto camera_mapping = pass->resources()->mapped_buffer(camera_buffer);
		camera_mapping[0] = scene_view.camera().viewproj_matrix();
	}

	auto transform_mapping = pass->resources()->mapped_buffer(transform_buffer);
	auto transforms = pass->resources()->buffer<BufferUsage::AttributeBit>(transform_buffer);
	const auto& descriptor_set = pass->descriptor_sets()[0];

	recorder.bind_attrib_buffers({transforms, transforms});

	const auto* material = ctx->resources()[EditorResources::ImGuiMaterialTemplate];
	recorder.bind_material(material, {});

	usize index = 0;
	for(const auto& [tr, me] : scene_view.world().view(StaticMeshArchetype())) {
		transform_mapping[index] = tr.transform();
		me.render(recorder, Renderable::SceneData{descriptor_set, u32(index)});
		++index;
	}

	return index;
}


DefaultEditorRenderer DefaultEditorRenderer::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const std::shared_ptr<IBLData>& ibl_data, const Settings& settings) {
	DefaultEditorRenderer renderer;
	renderer.renderer = DefaultRenderer::create(framegraph, view, size, ibl_data);

	auto camera_buffer = framegraph.declare_typed_buffer<math::Matrix4<>>();
	auto transform_buffer = framegraph.declare_typed_buffer<math::Transform<>>(max_batch_size);

	FrameGraphPassBuilder builder = framegraph.add_pass("Editor pass");
	builder.add_uniform_input(camera_buffer);
	builder.add_attrib_input(transform_buffer);
	builder.map_update(camera_buffer);
	builder.map_update(transform_buffer);

	builder.add_color_output(renderer.renderer.tone_mapping.tone_mapped, Framebuffer::LoadOp::Load);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			if(settings.enable_editor_entities) {
				render(ctx, render_pass, self, view, camera_buffer, transform_buffer);
			}
		});


	return renderer;
}

}
