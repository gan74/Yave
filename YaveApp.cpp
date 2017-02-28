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

#include <yave/image/ImageData.h>
#include <yave/buffer/TypedBuffer.h>

#include <y/io/File.h>

#include <iostream>


void wat(int) {}

namespace yave {

YaveApp::YaveApp(DebugParams params) : instance(params), device(instance), command_pool(&device), mesh_pool(&device) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMesh));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));
}

YaveApp::~YaveApp() {
	delete pipeline;

	delete scene_view;
	delete scene;

	materials.clear();
	mesh_texture = Texture();

	delete swapchain;
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::create_command_buffers() {
	/*for(const auto& img : swapchain->images()) {
		CmdBufferRecorder<CmdBufferUsage::Normal> recorder(command_pool.create_buffer());

		renderer->draw(recorder, img);

		command_buffers << recorder.end();
	}*/
}

void YaveApp::draw() {
	core::DebugTimer f("frame", core::Duration::milliseconds(8));

	auto vk_swap = swapchain->vk_swapchain();
	auto image_acquired_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	auto render_finished_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device.vk_queue(QueueFamily::Graphics);

	FrameToken frame = std::move(swapchain->next_frame(image_acquired_semaphore));

	//renderer->update();
	auto cmd_buffer =  CmdBufferRecorder<>(command_pool.create_buffer());

	{
		core::DebugTimer p("process", core::Duration::milliseconds(4));
		pipeline->process(frame, cmd_buffer);
	}

	{

		auto buffer = cmd_buffer.end().vk_cmd_buffer();
		auto submit_info = vk::SubmitInfo()
				.setWaitSemaphoreCount(0)
				.setPWaitDstStageMask(&pipe_stage_flags)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&buffer)
				.setSignalSemaphoreCount(1)
				.setPSignalSemaphores(&render_finished_semaphore);

		graphic_queue.submit(1, &submit_info, VK_NULL_HANDLE);

		graphic_queue.presentKHR(vk::PresentInfoKHR()
				.setSwapchainCount(1)
				.setPSwapchains(&vk_swap)
				.setPImageIndices(&frame.image_index)
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&render_finished_semaphore)
			);


	}
	graphic_queue.waitIdle();

	device.vk_device().destroySemaphore(image_acquired_semaphore);
	device.vk_device().destroySemaphore(render_finished_semaphore);

	if(command_pool.active_buffers() > 1) {
		fatal("Unable to collect command buffers.");
	}
}

void YaveApp::update(math::Vec2 angles) {
	float dist = 150;

	auto cam_pos =
			(math::rotation(angles.x(), math::Vec3(0, 0, -1)) *
			math::rotation(angles.y(), math::Vec3(0, 1, 0))) * math::Vec4(dist, 0, 0, 1);

	camera.set_view(math::look_at(cam_pos.sub(3) / cam_pos.w(), math::Vec3()));
	camera.set_proj(math::perspective(math::to_rad(45), 4.0f / 3.0f, 0.01f,  dist * 2));
}

void YaveApp::create_assets() {
	{
		{
			auto file = std::move(io::File::open("../tools/image_to_yt/chalet.jpg.yt").expected("Unable to load texture file."));
			auto image = ImageData::from_file(file);
			log_msg(core::String() + (image.size().x() * image.size().y()) + " pixels loaded");
			mesh_texture = Texture(&device, image);
		}
		for(usize i = 0; i != 128; ++i) {
			materials << asset_ptr(Material(&device, MaterialData()
					.set_frag_data(SpirVData::from_file(io::File::open("basic.frag.spv").expected("Unable to load spirv file")))
					.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file")))
					.set_bindings({Binding(TextureView(mesh_texture))})
				));
		}
	}


	core::Vector<const char*> meshes = {"../tools/obj_to_ym/cube.obj.ym"};
	core::Vector<StaticMesh> objects;
	for(auto name : meshes) {
		auto m_data = MeshData::from_file(io::File::open(name).expected("Unable to load mesh file"));
		log_msg(core::str() + m_data.triangles.size() + " triangles loaded");
		auto mesh = AssetPtr<StaticMeshInstance>(mesh_pool.create_static_mesh(/*new StaticMeshInstance(&device,*/ m_data));

		/*for(usize i = 0; i != 30; i++) {
			auto m = StaticMesh(mesh, material);
			m.set_position(math::Vec3(objects.size() * (objects.size() % 2 ? -4.0f : 4.0f), 0, 0));

			objects << std::move(m);
		}*/

		usize max = 30;
		for(usize x = 0; x != max; x++) {
			for(usize y = 0; y != max; y++) {
				for(usize z = 0; z != max; z++) {
					auto m = StaticMesh(mesh, materials[rand() % materials.size()]);
					m.set_position((math::Vec3(x, y, z) - math::Vec3(max) * 0.5f) * 5.0f);
					objects << std::move(m);
				}
			}
		}
	}
	scene = new Scene(std::move(objects));
	scene_view = new SceneView(*scene, camera);

	update();

	{
		auto culling = core::Arc<CullingNode>(new CullingNode(*scene_view));
		auto gbuffer = core::Arc<GBufferRenderer>(new GBufferRenderer(&device, swapchain->size(), culling));
		auto deferred = core::Arc<Node>(new DeferredRenderer(gbuffer));

		pipeline = new Pipeline(deferred);
	}

	//renderer = new DeferredRenderer(*scene_view, swapchain->size());

}

}

