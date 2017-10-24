/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include "YaveApp.h"

#include <yave/images/ImageData.h>
#include <yave/buffers/TypedBuffer.h>
#include <yave/renderers/RenderingPipeline.h>

#include <y/io/File.h>

#include <iostream>

namespace yave {

static core::Chrono time;

YaveApp::YaveApp(DebugParams params) : instance(params), device(instance) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMeshInstance));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));
	log_msg("sizeof(DeviceMemory) = "_s + sizeof(DeviceMemory));
	log_msg("sizeof(Texture) = "_s + sizeof(Texture));
	log_msg("sizeof(TextureView) = "_s + sizeof(TextureView));

	create_assets();
}

YaveApp::~YaveApp() {
	device.vk_queue(QueueFamily::Graphics).waitIdle();

	delete scene_view;
	delete scene;

	swapchain = nullptr;
}

void YaveApp::draw() {
	core::DebugTimer f("frame", core::Duration::milliseconds(8));

	FrameToken frame = swapchain->next_frame();

	CmdBufferRecorder<> recorder(device.create_cmd_buffer());

	{
		core::DebugTimer _("pipeline", core::Duration::milliseconds(6));
		RenderingPipeline pipeline(frame);
		pipeline.dispatch(renderer, recorder);
	}

	RecordedCmdBuffer<> cmd_buffer(std::move(recorder));

	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device.vk_queue(QueueFamily::Graphics);
	auto vk_buffer = cmd_buffer.vk_cmd_buffer();

	cmd_buffer.wait();
	device.vk_device().resetFences(cmd_buffer.vk_fence());
	graphic_queue.submit(vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&frame.image_aquired)
			.setPWaitDstStageMask(&pipe_stage_flags)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&vk_buffer)
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&frame.render_finished),
		cmd_buffer.vk_fence());

	swapchain->present(frame, graphic_queue);

	//graphic_queue.waitIdle();
}

void YaveApp::update(math::Vec2 angles) {
	float dist = 125.0f;

	auto cam_tr = math::rotation({0, 0, -1}, angles.x()) * math::rotation({0, 1, 0}, angles.y());
	auto cam_pos = cam_tr * math::Vec4(dist, 0, 0, 1);
	auto cam_up = cam_tr * math::Vec4(0, 0, 1, 0);

	camera.set_view(math::look_at(cam_pos.to<3>() / cam_pos.w(), math::Vec3(), cam_up.to<3>()));
	camera.set_proj(math::perspective(math::to_rad(60.0f), 4.0f / 3.0f, 0.1f,  10000.0f));
}

void YaveApp::create_assets() {
	core::Vector<Scene::Ptr<StaticMeshInstance>> objects;
	core::Vector<Scene::Ptr<Renderable>> renderables;
	core::Vector<Scene::Ptr<Light>> lights;

	{
		Light l(Light::Directional);
		l.transform().set_basis(math::Vec3{1.0f, 1.0f, 3.0f}.normalized(), {1.0f, 0.0f, 0.0f});
		l.color() = math::Vec3{1.0f};
		lights << std::move(l);
	}
	/*{
		Light l(Light::Point);
		l.position() = math::Vec3{0.0f, 2.5f, 0.0f};
		l.color() = math::Vec3{5.0f, 0.0f, 0.0f};
		l.radius() = 10.0f;
		lights << std::move(l);
	}*/

	{


		/*{
			core::Vector<AssetPtr<Texture>> textures;

			auto test_data = ImageData::from_file(io::File::open("../tools/image_to_yt/16.yt").expected("Unable to load image file."));
			textures << AssetPtr<Texture>(Texture(&device, test_data));

			auto color_data = ImageData::from_file(io::File::open("../tools/image_to_yt/Cerberus/albedo.yt").expected("Unable to load image file."));
			for(usize i = 0; i != 64; ++i) {
				auto color = AssetPtr<Texture>(Texture(&device, color_data));
				unused(color);
			}
		}*/

		auto color_data = ImageData::from_file(io::File::open("../tools/image_to_yt/Cerberus/albedo.yt").expected("Unable to load image file."));
		auto color = AssetPtr<Texture>(Texture(&device, color_data));
		destroy_later(color);

		auto roughness_data = ImageData::from_file(io::File::open("../tools/image_to_yt/Cerberus/roughness.yt").expected("Unable to load image file."));
		auto roughness = AssetPtr<Texture>(Texture(&device, roughness_data));
		destroy_later(roughness);

		auto metallic_data = ImageData::from_file(io::File::open("../tools/image_to_yt/Cerberus/metallic.yt").expected("Unable to load image file."));
		auto metallic = AssetPtr<Texture>(Texture(&device, metallic_data));
		destroy_later(metallic);

		auto normal_data = ImageData::from_file(io::File::open("../tools/image_to_yt/Cerberus/normal.yt").expected("Unable to load image file."));
		auto normal = AssetPtr<Texture>(Texture(&device, normal_data));
		destroy_later(normal);

		auto material = AssetPtr<Material>(Material(&device, MaterialData()
				 .set_frag_data(SpirVData::from_file(io::File::open("textured.frag.spv").expected("Unable to load spirv file.")))
				 .set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
				 .set_bindings({Binding(*color), Binding(*roughness), Binding(*metallic), Binding(*normal)})
			 ));

		auto mesh_data = MeshData::from_file(io::File::open("../tools/mesh_to_ym/Cerberus.FBX.ym").expected("Unable to open mesh file."));
		auto mesh = AssetPtr<StaticMesh>(StaticMesh(&device, mesh_data));

		log_msg(core::str(mesh_data.triangles().size()) + " triangles loaded");

		{
			auto instance = new StaticMeshInstance(mesh, material);
			instance->transform().set_basis({-1, 0, 0}, {0, 0, -1}, {0, -1, 0});
			instance->position() = {0, -40, 0};
			renderables << instance;
		}
		/*{
			auto instance = new StaticMeshInstance(mesh, material);
			instance->position() = {0, 5, 0};
			renderables << instance;
		}
		{
			auto instance = new StaticMeshInstance(mesh, material);
			renderables << instance;
		}*/
	}

	scene = new Scene(std::move(objects), std::move(renderables), std::move(lights));
	scene_view = new SceneView(*scene, camera);

	update();

	device.allocator().dump_info();
}


void YaveApp::create_renderers() {
	auto culling = core::Arc<CullingNode>(new CullingNode(*scene_view));
	auto gbuffer = core::Arc<GBufferRenderer>(new GBufferRenderer(&device, swapchain->size(), culling));
	//auto depth = core::Arc<BufferRenderer>(new DepthRenderer(&device, swapchain->size(), culling));
	auto deferred = core::Arc<BufferRenderer>(new DeferredRenderer(gbuffer));
	renderer = core::Arc<EndOfPipeline>(new ColorCorrectionRenderer(deferred));
}

}

