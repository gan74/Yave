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

#include "SceneRenderSubPass.h"

namespace yave {

static constexpr usize max_batch_size = 128 * 1024;

SceneRenderSubPass SceneRenderSubPass::create(FrameGraph& framegraph, FrameGraphPassBuilder& builder, const SceneView* view) {
	auto camera_buffer = framegraph.declare_typed_buffer<math::Matrix4<>>();
	auto transform_buffer = framegraph.declare_typed_buffer<math::Transform<>>(max_batch_size);

	SceneRenderSubPass pass;
	pass.scene_view = view;
	pass.camera_buffer = camera_buffer;
	pass.transform_buffer = transform_buffer;

	builder.add_uniform_input(camera_buffer);
	builder.add_attrib_input(transform_buffer);
	builder.map_update(camera_buffer);
	builder.map_update(transform_buffer);

	return pass;
}


void SceneRenderSubPass::render(RenderPassRecorder& recorder, const FrameGraphPass* pass) const {
	y_profile();

	auto& descriptor_set = pass->descriptor_sets()[0];

	// fill render data
	{
		auto camera_mapping = pass->resources()->mapped_buffer(camera_buffer);
		camera_mapping[0] = scene_view->camera().viewproj_matrix();
	}

	usize attrib_index = 0;
	{
		auto transform_mapping = pass->resources()->mapped_buffer(transform_buffer);
		if(transform_mapping.size() < scene_view->scene().renderables().size() + scene_view->scene().static_meshes().size()) {
			y_fatal("Transform buffer overflow.");
		}

		{
			// renderables
			for(const auto& r : scene_view->scene().renderables()) {
				transform_mapping[attrib_index++] = r->transform();
			}

			// static meshes
			for(const auto& r : scene_view->scene().static_meshes()) {
				transform_mapping[attrib_index++] = r->transform();
			}
		}
	}

	// render stuff
	if(attrib_index) {
		u32 attrib_index = 0;

		auto transforms = pass->resources()->buffer<BufferUsage::AttributeBit>(transform_buffer);
		recorder.bind_attrib_buffers({transforms, transforms});

		// renderables
		{
			for(const auto& r : scene_view->scene().renderables()) {
				r->render(recorder, Renderable::SceneData{descriptor_set, attrib_index++});
			}
		}

		// static meshes
		{
			for(const auto& r : scene_view->scene().static_meshes()) {
				r->render(recorder, Renderable::SceneData{descriptor_set, attrib_index++});
			}
		}
	}
}

}
