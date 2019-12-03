/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "LightingPass.h"

#include <yave/device/Device.h>
#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/entities/entities.h>

#include <yave/meshes/StaticMesh.h>

#include <y/core/Chrono.h>
#include <y/io2/File.h>

namespace yave {

static constexpr usize max_directional_lights = 16;
static constexpr usize max_point_lights = 1024;

static constexpr usize tile_size = 64;
static constexpr usize max_lights_per_cluster = 64;

static constexpr math::Vec3ui compute_cluster_count(const math::Vec2ui& size) {
	math::Vec2ui tiles;
	for(usize i = 0; i != 2; ++i) {
		tiles[i] = size[i] / tile_size;
		if(tiles[i] * tile_size < size[i]) {
			++tiles[i];
		}
	}
	return math::Vec3ui(tiles, 16);
}

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const std::shared_ptr<IBLProbe>& ibl_probe, bool use_clustered_renderer) {
	static constexpr vk::Format lighting_format = vk::Format::eR16G16B16A16Sfloat;
	const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

	const SceneView& scene = gbuffer.scene_pass.scene_view;
	const Camera camera = scene.camera();

	struct PushData {
		uniform::LightingCamera camera;
		u32 light_count = 0;
	};

	FrameGraphPassBuilder ambient_builder = framegraph.add_pass("Ambient/Sun pass");
	const auto lit = ambient_builder.declare_image(lighting_format, size);
	{
		const auto directional_buffer = ambient_builder.declare_typed_buffer<uniform::DirectionalLight>(max_directional_lights);

		ambient_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(*ibl_probe, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(framegraph.device()->device_resources().brdf_lut(), 0, PipelineStage::ComputeBit);
		ambient_builder.add_storage_input(directional_buffer, 0, PipelineStage::ComputeBit);
		ambient_builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
		ambient_builder.map_update(directional_buffer);

		ambient_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			PushData push_data{camera, 0};
			TypedMapping<uniform::DirectionalLight> mapping = self->resources().mapped_buffer(directional_buffer);
			for(auto [l] : scene.world().view(DirectionalLightArchetype()).components()) {
				mapping[push_data.light_count++] = {
						-l.direction().normalized(),
						0,
						l.color() * l.intensity(),
						0
					};
			}

			const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredAmbientProgram];
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
		});
	}



	if(use_clustered_renderer) {
		const math::Vec3ui cluster_count = compute_cluster_count(size);
		const usize total_clusters = cluster_count.x() * cluster_count.y() * cluster_count.z();

		struct ClusteringData {
			uniform::LightingCamera camera;
			math::Matrix4<> view_proj;
			math::Vec3ui cluster_count;
		};


		FrameGraphPassBuilder clear_builder = framegraph.add_pass("Cluster clearing pass");

		const auto tile_buffer = clear_builder.declare_image(vk::Format::eR32Uint, cluster_count.to<2>() * math::Vec2ui(1, cluster_count.z()));

		clear_builder.add_color_output(tile_buffer);
		clear_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			recorder.bind_framebuffer(self->framebuffer());
		});


		FrameGraphPassBuilder cluster_builder = framegraph.add_pass("Clustering pass");

		const auto light_buffer = cluster_builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);
		const auto index_buffer = cluster_builder.declare_typed_buffer<u32>(max_lights_per_cluster * total_clusters);
		const auto cluster_buffer = cluster_builder.declare_typed_buffer<ClusteringData>();
		const auto tiles = cluster_builder.declare_image(vk::Format::eR8Unorm, cluster_count.to<2>());

		cluster_builder.add_uniform_input(cluster_buffer);
		cluster_builder.add_storage_input(light_buffer);
		cluster_builder.add_storage_output(index_buffer);
		cluster_builder.add_storage_output(tile_buffer);

		cluster_builder.map_update(cluster_buffer);
		cluster_builder.add_color_output(tiles);

		cluster_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			usize light_count = 0;
			{
				auto mapping = self->resources().mapped_buffer(light_buffer);
				for(auto [t, l] : scene.world().view(PointLightArchetype()).components()) {
					mapping[light_count++] = {
							t.position(),
							l.radius(),
							l.color() * l.intensity(),
							std::max(math::epsilon<float>, l.falloff())
					};
				}
			}
			{
				auto mapping = self->resources().mapped_buffer(cluster_buffer);
				mapping[0] = {
					camera,
					camera.viewproj_matrix(),
					cluster_count
				};
			}

			const DeviceResources& res = recorder.device()->device_resources();
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			const StaticMesh& sphere = *res[DeviceResources::SimpleSphereMesh];
			auto indirect = sphere.indirect_data();
			render_pass.bind_material(res[DeviceResources::ClusterBuilderMaterialTemplate], {self->descriptor_sets()[0]});
			render_pass.bind_buffers(sphere.triangle_buffer(), sphere.vertex_buffer());

			const bool use_instancing = true;
			if(use_instancing) {
				indirect.setInstanceCount(light_count);
				render_pass.draw(indirect);
			} else {
				for(usize i = 0; i != light_count; ++i) {
					indirect.setFirstInstance(i);
					render_pass.draw(indirect);
				}
			}

		});


		FrameGraphPassBuilder local_builder = framegraph.add_pass("Lighting pass");

		local_builder.add_uniform_input(gbuffer.depth);
		local_builder.add_uniform_input(gbuffer.color);
		local_builder.add_uniform_input(gbuffer.normal);
		local_builder.add_uniform_input(tile_buffer);
		local_builder.add_storage_input(light_buffer);
		local_builder.add_storage_input(index_buffer);
		local_builder.add_uniform_input(cluster_buffer);

		local_builder.add_color_output(lit);
		local_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			const auto* material = recorder.device()->device_resources()[DeviceResources::ClusteredLocalsMaterialTemplate];
			render_pass.bind_material(material, {self->descriptor_sets()[0]});
			render_pass.draw(vk::DrawIndirectCommand(6, 1));
		});

	} else {

		FrameGraphPassBuilder point_builder = framegraph.add_pass("Lighting pass");

		const auto light_buffer = point_builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);

		point_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
		point_builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
		point_builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
		point_builder.add_storage_input(light_buffer, 0, PipelineStage::ComputeBit);
		point_builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
		point_builder.map_update(light_buffer);

		point_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			PushData push_data{camera, 0};
			TypedMapping<uniform::PointLight> mapping = self->resources().mapped_buffer(light_buffer);
			for(auto [t, l] : scene.world().view(PointLightArchetype()).components()) {
				mapping[push_data.light_count++] = {
						t.position(),
						l.radius(),
						l.color() * l.intensity(),
						std::max(math::epsilon<float>, l.falloff())
					};
			}

			if(push_data.light_count) {
				const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredLocalsProgram];
				recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
			}
		});
	}

	LightingPass pass;
	pass.lit = lit;
	return pass;
}

}
