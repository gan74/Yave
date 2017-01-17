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
}

void YaveApp::init(Window* window) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMesh));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));

	swapchain = new Swapchain(&device, window);

	create_assets();

	create_command_buffers();
}

YaveApp::~YaveApp() {
	delete renderer;

	delete scene_view;
	delete scene;

	material = nullptr;
	mesh_texture = Texture();

	delete swapchain;

	command_buffers.clear();
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::create_command_buffers() {
	for(usize i = 0; i != swapchain->image_count(); i++) {
		CmdBufferRecorder<CmdBufferUsage::Normal> recorder(command_pool.create_buffer());

		renderer->draw(recorder, swapchain->image(i));

		command_buffers << recorder.end();
	}
}

core::Duration YaveApp::draw() {
	core::Chrono ch;

	auto vk_swap = swapchain->vk_swapchain();
	auto image_acquired_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	auto render_finished_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	u32 image_index = device.vk_device().acquireNextImageKHR(vk_swap, u64(-1), image_acquired_semaphore, VK_NULL_HANDLE).value;
	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device.vk_queue(QueueFamily::Graphics);

	renderer->update();
	{

		auto buffer = command_buffers[image_index].vk_cmd_buffer();
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
				.setPImageIndices(&image_index)
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&render_finished_semaphore)
			);


	}
	graphic_queue.waitIdle();

	device.vk_device().destroySemaphore(image_acquired_semaphore);
	device.vk_device().destroySemaphore(render_finished_semaphore);
	return ch.elapsed();
}

void YaveApp::update(math::Vec2 angles) {
	auto cam_pos =
			(math::rotation(angles.x(), math::Vec3(0, 0, -1)) *
			math::rotation(angles.y(), math::Vec3(0, 1, 0))) * math::Vec4(2.5, 0, 0, 1);


	camera.set_view(math::look_at(cam_pos.sub(3) / cam_pos.w(), math::Vec3()));
}

void YaveApp::create_assets() {
	{
		{
			auto file = std::move(io::File::open("../tools/image_to_yt/chalet.jpg.yt").expected("Unable to load texture file."));
			auto image = ImageData::from_file(file);
			log_msg(core::String() + (image.size().x() * image.size().y()) + " pixels loaded");
			mesh_texture = Texture(&device, vk::Format::eR8G8B8A8Unorm, image);
		}
		material = asset_ptr(Material(&device, MaterialData()
				.set_frag_data(SpirVData::from_file(io::File::open("basic.frag.spv").expected("Unable to load spirv file")))
				.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file")))
				.set_bindings({Binding(TextureView(mesh_texture))})
			));
	}


	core::Vector<const char*> meshes = {"../tools/obj_to_ym/chalet.obj.ym"};
	core::Vector<StaticMesh> objects;
	for(auto name : meshes) {
		auto m_data = MeshData::from_file(io::File::open(name).expected("Unable to load mesh file"));
		log_msg(core::str() + m_data.triangles.size() + " triangles loaded");
		auto mesh = AssetPtr<StaticMeshInstance>(mesh_pool.create_static_mesh(m_data));
		for(usize i = 0; i != 1; i++) {
			auto m = StaticMesh(mesh, material);
			m.set_position(math::Vec3(objects.size() * -3.0f, 0, 0));

			objects << std::move(m);
		}
	}
	scene = new Scene(std::move(objects));
	scene_view = new SceneView(&device, *scene, camera);

	update();

	renderer = new DeferredRenderer(*scene_view, swapchain->size());

}



}

