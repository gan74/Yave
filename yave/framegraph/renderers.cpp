/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "renderers.h"

#include <y/io/File.h>

namespace yave {

static constexpr usize max_batch_size = 128 * 1024;

SceneRenderSubPass create_scene_render(FrameGraph& framegraph, FrameGraphPassBuilder& builder, const SceneView* view) {
	SceneRenderSubPass pass;
	pass.camera_buffer = framegraph.declare_typed_buffer<math::Matrix4<>>();
	pass.transform_buffer = framegraph.declare_typed_buffer<math::Transform<>>(max_batch_size);
	pass.scene_view = view;

	builder.add_uniform_input(pass.camera_buffer);
	builder.add_attrib_input(pass.transform_buffer);
	builder.map_update(pass.camera_buffer);
	builder.map_update(pass.transform_buffer);

	return pass;
}


void render_scene(RenderPassRecorder& recorder, const SceneRenderSubPass& subpass, const FrameGraphPass* pass) {
	auto& descriptor_set = pass->descriptor_sets()[0];


	// fill render data
	{
		auto camera_mapping = pass->resources()->get_mapped_buffer(subpass.camera_buffer);
		camera_mapping[0] = subpass.scene_view->camera().viewproj_matrix();
	}

	{
		auto transform_mapping = pass->resources()->get_mapped_buffer(subpass.transform_buffer);
		if(transform_mapping.size() < subpass.scene_view->scene().renderables().size() + subpass.scene_view->scene().static_meshes().size()) {
			y_fatal("Transform buffer overflow.");
		}

		u32 attrib_index = 0;
		{
			// renderables
			for(const auto& r : subpass.scene_view->scene().renderables()) {
				transform_mapping[attrib_index++] = r->transform();
			}

			// static meshes
			for(const auto& r : subpass.scene_view->scene().static_meshes()) {
				transform_mapping[attrib_index++] = r->transform();
			}
		}
	}

	// render stuff
	{
		u32 attrib_index = 0;

#warning clean unnecessary buffer binding
		auto transform_buffer = pass->resources()->get_buffer<BufferUsage::AttributeBit>(subpass.transform_buffer);
		recorder.bind_attrib_buffers({transform_buffer, transform_buffer});

		// renderables
		{
			for(const auto& r : subpass.scene_view->scene().renderables()) {
				r->render(recorder, Renderable::SceneData{descriptor_set, attrib_index++});
			}
		}

		// static meshes
		{
			for(const auto& r : subpass.scene_view->scene().static_meshes()) {
				r->render(recorder, Renderable::SceneData{descriptor_set, attrib_index++});
			}
		}
	}
}


GBufferPass render_gbuffer(FrameGraph& framegraph, const SceneView* view, const math::Vec2ui& size) {
	static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
	static constexpr vk::Format color_format = vk::Format::eR8G8B8A8Unorm;
	static constexpr vk::Format normal_format = vk::Format::eR16G16B16A16Unorm;

	FrameGraphPassBuilder builder = framegraph.add_pass("G-buffer pass");

	GBufferPass pass;
	auto depth = framegraph.declare_image(depth_format, size);
	auto color = framegraph.declare_image(color_format, size);
	auto normal = framegraph.declare_image(normal_format, size);

	pass.depth = depth;
	pass.color = color;
	pass.normal = normal;
	pass.scene_pass = create_scene_render(framegraph, builder, view);

	builder.add_depth_output(depth);
	builder.add_color_output(color);
	builder.add_color_output(normal);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			render_scene(render_pass, pass.scene_pass, self);
		});

	return pass;
}



static const ComputeProgram& create_lighting_shader(DevicePtr dptr) {
	static std::unique_ptr<ComputeProgram> prog;
	if(!prog) {
		prog = std::make_unique<ComputeProgram>(ComputeShader(dptr, SpirVData::deserialized(io::File::open("framegraphlight.comp.spv").expected("Unable to open SPIR-V file."))));
	}
	return *prog;
}

LightingPass render_lighting(FrameGraph& framegraph, const GBufferPass& gbuffer) {
	static constexpr vk::Format lighting_format = vk::Format::eR16G16B16A16Sfloat;

	math::Vec2ui size = framegraph.image_size(gbuffer.depth);
	uniform::Camera camera_data = gbuffer.scene_pass.scene_view->camera();

	FrameGraphPassBuilder builder = framegraph.add_pass("Lighting pass");

	LightingPass pass;
	auto lighting = framegraph.declare_image(lighting_format, size);

	pass.lighting = lighting;

	builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
	builder.add_storage_output(lighting);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			const auto& program = create_lighting_shader(recorder.device());
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, camera_data);
		});

	return pass;
}

}
