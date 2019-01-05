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
#include <yave/material/Material.h>

namespace yave {

static constexpr usize max_batch_size = 128 * 1024;
static constexpr usize max_light_count = 1024;

SceneRenderSubPass create_scene_render(FrameGraph& framegraph, FrameGraphPassBuilder& builder, const SceneView* view) {
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

	auto depth = framegraph.declare_image(depth_format, size);
	auto color = framegraph.declare_image(color_format, size);
	auto normal = framegraph.declare_image(normal_format, size);

	GBufferPass pass;
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
		prog = std::make_unique<ComputeProgram>(ComputeShader(dptr, SpirVData::deserialized(io::File::open("deferred.comp.spv").expected("Unable to open SPIR-V file."))));
	}
	return *prog;
}

LightingPass render_lighting(FrameGraph& framegraph, const GBufferPass& gbuffer, const std::shared_ptr<IBLData>& ibl_data) {
	static constexpr vk::Format lighting_format = vk::Format::eR16G16B16A16Sfloat;
	math::Vec2ui size = framegraph.image_size(gbuffer.depth);

	const SceneView* scene = gbuffer.scene_pass.scene_view;

	FrameGraphPassBuilder builder = framegraph.add_pass("Lighting pass");

	auto lit = framegraph.declare_image(lighting_format, size);
	auto light_buffer = framegraph.declare_typed_buffer<uniform::Light>(max_light_count);

	LightingPass pass;
	pass.lit = lit;

	builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(ibl_data->envmap(), 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(ibl_data->brdf_lut(), 0, PipelineStage::ComputeBit);
	builder.add_storage_input(light_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);

	builder.map_update(light_buffer);

	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			struct PushData {
				uniform::Camera camera;
				u32 point_count = 0;
				u32 directional_count = 0;
			} push_data;
			push_data.camera = scene->camera();

			TypedMapping<uniform::Light> mapping = self->resources()->get_mapped_buffer(light_buffer);
			usize light_count = scene->scene().lights().size();
			for(const auto& l : scene->scene().lights()) {
				if(l->type() == Light::Point) {
					mapping[push_data.point_count++] = *l;
				} else {
					mapping[light_count - ++push_data.directional_count] = *l;
				}
			}

			const auto& program = create_lighting_shader(recorder.device());
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
		});

	return pass;
}

static const Material& create_tone_mapping_material(DevicePtr dptr) {
	static std::unique_ptr<Material> mat;
	if(!mat) {
		mat = std::make_unique<Material>(dptr, MaterialData()
			.set_frag_data(SpirVData::deserialized(io::File::open("tonemap.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::deserialized(io::File::open("screen.vert.spv").expected("Unable to load spirv file.")))
			.set_depth_tested(false)
		);
	}
	return *mat;
}

ToneMappingPass render_tone_mapping(FrameGraph& framegraph, const LightingPass& lighting) {
	static constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;
	math::Vec2ui size = framegraph.image_size(lighting.lit);

	FrameGraphPassBuilder builder = framegraph.add_pass("Tone mapping pass");

	auto tone_mapped = framegraph.declare_image(format, size);

	ToneMappingPass pass;
	pass.tone_mapped = tone_mapped;

	builder.add_color_output(tone_mapped);
	builder.add_uniform_input(lighting.lit, 0, PipelineStage::ComputeBit);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			render_pass.bind_material(create_tone_mapping_material(recorder.device()), {self->descriptor_sets()[0]});
			render_pass.draw(vk::DrawIndirectCommand(6, 1));
		});

	return pass;
}

}
