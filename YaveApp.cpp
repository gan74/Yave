/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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


	swapchain = new Swapchain(&device, window);

	create_mesh();

	create_graphic_pipeline();

	create_command_buffers();
}

YaveApp::~YaveApp() {
	mesh_texture = Texture();
	static_mesh = StaticMeshInstance();

	delete swapchain;

	command_buffers.clear();
}


void YaveApp::create_graphic_pipeline() {
	uniform_buffer = TypedBuffer<MVP, BufferUsage::UniformBuffer, MemoryFlags::CpuVisible>(&device, 1);

	material = Material(&device, MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("frag.spv")))
			.set_vert_data(SpirVData::from_file(io::File::open("vert.spv")))
			.set_uniform_buffers(core::vector(UniformBinding(0, uniform_buffer)))
			.set_textures(core::vector(TextureBinding(1, TextureView(mesh_texture))))
		);
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::create_command_buffers() {
	for(usize i = 0; i != swapchain->buffer_count(); i++) {

		CmdBufferRecorder recorder(command_pool.create_buffer());
		recorder.bind_framebuffer(swapchain->get_framebuffer(i));
		recorder.bind_pipeline(material.compile(swapchain->get_render_pass(), Viewport(swapchain->size())));
		recorder.draw(static_mesh);
		recorder.draw(static_mesh2);
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
	auto mapping = uniform_buffer.map();
	auto& mvp = *mapping.begin();

	auto ratio = swapchain->size().x() / float(swapchain->size().y());
	mvp.proj = math::perspective(math::to_rad(45), ratio, 0.001f, 2.5f);
	mvp.view = math::look_at(math::Vec3(1.5, 0, 0), math::Vec3());
	mvp.model = math::rotation(angles.x(), math::Vec3(0, 0, 1)) *
				math::rotation(angles.y(), math::Vec3(0, 1, 0));
}

void YaveApp::create_mesh() {
	{
		auto file = io::File::open("../tools/mesh/chalet.ym");
		auto m_data = MeshData::from_file(file);
		log_msg(core::String() + m_data.triangles.size() + " triangles loaded");
		static_mesh = StaticMeshInstance(&device, m_data);
	}
	{
		auto file = io::File::open("../tools/mesh/cube.ym");
		auto m_data = MeshData::from_file(file);
		log_msg(core::String() + m_data.triangles.size() + " triangles loaded");
		static_mesh2 = StaticMeshInstance(&device, m_data);
	}
	{
		auto file = io::File::open("../tools/image/chalet.jpg.rgba");
		auto image = ImageData::from_file(file);
		log_msg(core::String() + (image.size().x() * image.size().y()) + " pixels loaded");
		mesh_texture = Texture(&device, vk::Format::eR8G8B8A8Unorm, image.size(), image.get_raw_data());
	}
}



}

