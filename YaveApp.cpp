/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#include "YaveApp.h"

#include <yave/image/ImageData.h>
#include <yave/buffer/TypedBuffer.h>

#include <y/io/File.h>


namespace yave {

YaveApp::YaveApp(DebugParams params) : instance(params), device(instance), command_pool(&device) {
}

void YaveApp::init(Window* window) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMesh));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));

	swapchain = new Swapchain(&device, window);

	create_assets();

	create_command_buffers();
}

YaveApp::~YaveApp() {
	//range(objects).foreach([](auto ptr) { delete ptr; });
	objects.clear();
	material = nullptr;
	mesh_texture = Texture();

	delete swapchain;

	command_buffers.clear();

	mvp_set = DescriptorSet();
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::create_command_buffers() {
	for(usize i = 0; i != swapchain->buffer_count(); i++) {

		CmdBufferRecorder recorder(command_pool.create_buffer());
		recorder.bind_framebuffer(swapchain->get_framebuffer(i));
		recorder.set_viewport(Viewport(swapchain->size()));
		for(auto& obj : objects) {
			obj.draw(recorder, mvp_set);
		}
		command_buffers << recorder.end();
	}
}

Duration YaveApp::draw() {
	Chrono ch;
	auto vk_swap = swapchain->get_vk_swapchain();
	auto image_acquired_semaphore = device.get_vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	auto render_finished_semaphore = device.get_vk_device().createSemaphore(vk::SemaphoreCreateInfo());

	u32 image_index = device.get_vk_device().acquireNextImageKHR(vk_swap, u64(-1), image_acquired_semaphore, VK_NULL_HANDLE).value;

	auto buffer = command_buffers[image_index].get_vk_cmd_buffer();
	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto submit_info = vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&image_acquired_semaphore)
			.setPWaitDstStageMask(&pipe_stage_flags)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&buffer)
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&render_finished_semaphore);

	auto graphic_queue = device.get_vk_queue(QueueFamily::Graphics);

	graphic_queue.submit(1,& submit_info, VK_NULL_HANDLE);

	graphic_queue.presentKHR(vk::PresentInfoKHR()
			.setSwapchainCount(1)
			.setPSwapchains(&vk_swap)
			.setPImageIndices(&image_index)
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&render_finished_semaphore)
		);

	graphic_queue.waitIdle();

	device.get_vk_device().destroySemaphore(image_acquired_semaphore);
	device.get_vk_device().destroySemaphore(render_finished_semaphore);

	return ch.elapsed();
}

void YaveApp::update(math::Vec2 angles) {
	/*const_cast<math::Matrix4<>&>*/(objects.first().transform()) =
				math::rotation(angles.x(), math::Vec3(0, 0, 1)) *
				math::rotation(angles.y(), math::Vec3(0, 1, 0));
}

void YaveApp::create_assets() {
	uniform_buffer = TypedBuffer<MVP, BufferUsage::UniformBuffer, MemoryFlags::CpuVisible>(&device, 1);
	{
		{
			auto file = io::File::open("../tools/image/chalet.jpg.rgba");
			auto image = ImageData::from_file(file);
			log_msg(core::String() + (image.size().x() * image.size().y()) + " pixels loaded");
			mesh_texture = Texture(&device, vk::Format::eR8G8B8A8Unorm, image.size(), image.get_raw_data());
		}
		material = asset_ptr(Material(&device, MaterialData()
				.set_frag_data(SpirVData::from_file(io::File::open("frag.spv")))
				.set_vert_data(SpirVData::from_file(io::File::open("vert.spv")))
				.set_bindings(core::vector(Binding(TextureView(mesh_texture))))
			));
	}

	mvp_set = DescriptorSet(&device, {Binding(uniform_buffer)});

	core::Vector<const char*> meshes = {"../tools/mesh/chalet.ym"/*, "../tools/mesh/cube.ym"*/};
	for(auto name : meshes) {
		auto m_data = MeshData::from_file(io::File::open(name));
		log_msg(core::String() + m_data.triangles.size() + " triangles loaded");
		auto mesh = AssetPtr<StaticMeshInstance>(StaticMeshInstance(&device, m_data));
		objects << StaticMesh(mesh, material);
	}

	auto mapping = uniform_buffer.map();
	auto& mvp = *mapping.begin();

	auto ratio = swapchain->size().x() / float(swapchain->size().y());
	mvp.proj = math::perspective(math::to_rad(45), ratio, 0.001f, 2.5f);
	mvp.view = math::look_at(math::Vec3(1.5, 0, 0), math::Vec3());
}



}

