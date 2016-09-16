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
#include "LowLevelGraphics.h"
#include "Window.h"

#include <yave/image/ImageData.h>
#include <yave/buffer/TypedBuffer.h>

#include <y/io/File.h>

#include <iostream>

namespace yave {


LowLevelGraphics::LowLevelGraphics(DebugParams params) : instance(params), device(instance, this), command_pool(&device) {
}

void LowLevelGraphics::init(Window* window) {
	material_compiler = new MaterialCompiler(&device);

	set_surface(window);

	create_mesh();

	create_swapchain();
	create_graphic_pipeline();

	create_command_buffers();
}

LowLevelGraphics::~LowLevelGraphics() {
	mesh_texture = Texture();
	static_mesh = StaticMeshInstance();
	pipeline = GraphicPipeline();
	uniform_buffer = nullptr;

	delete material_compiler;
	delete swapchain;

	command_buffers.clear();

	instance.get_vk_instance().destroySurfaceKHR(surface);
}

void LowLevelGraphics::set_surface(Window* window) {
	#ifdef Y_OS_WIN
		surface = instance.get_vk_instance().createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR()
				.setHinstance(window->instance())
				.setHwnd(window->handle())
			);

		if(!device.has_wsi_support(surface)) {
			fatal("No WSI support");
		}
		std::cout << "Vulkan WSI supported !" << std::endl;
	#endif
}

void LowLevelGraphics::create_swapchain() {
	swapchain = new Swapchain(&device, surface);
}


void LowLevelGraphics::create_graphic_pipeline() {
	uniform_buffer = core::rc(TypedBuffer<MVP, BufferUsage::UniformBuffer, MemoryFlags::CpuVisible>(&device, 1));
	pipeline = material_compiler->compile(Material()
			.set_frag_data(SpirVData::from_file("frag.spv"))
			.set_vert_data(SpirVData::from_file("vert.spv"))
			.set_geom_data(SpirVData::from_file("geom.spv"))
			.set_uniform_buffers(core::vector(UniformBinding(0, uniform_buffer)))
			.set_textures(core::vector(TextureBinding(1, TextureView(mesh_texture))))
		, swapchain->get_render_pass(), Viewport(swapchain->size()));
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void LowLevelGraphics::create_command_buffers() {
	for(usize i = 0; i != swapchain->buffer_count(); i++) {
		/*auto begin_info = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
			;

		command_buffer->get_vk_cmd_buffer().begin(&begin_info);*/

		CmdBufferRecorder recorder(command_pool.create_buffer());
		recorder.bind_framebuffer(swapchain->get_framebuffer(i));
		recorder.bind_pipeline(pipeline);
		recorder.draw(static_mesh);
		command_buffers << recorder.end();
	}
}

void LowLevelGraphics::draw() {
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
}

void LowLevelGraphics::update(math::Vec2 angles) {
	auto mapping = uniform_buffer->map();
	auto& mvp = *mapping.begin();

	auto ratio = swapchain->size().x() / float(swapchain->size().y());
	mvp.proj = math::perspective(math::to_rad(45), ratio, 0.001f, 2.5f);
	mvp.view = math::look_at(math::Vec3(1.5, 0, 0), math::Vec3());
	mvp.model = math::rotation(angles.x(), math::Vec3(0, 0, 1)) *
				math::rotation(angles.y(), math::Vec3(0, 1, 0));
}


/*DisposableCmdBuffer LowLevelGraphics::create_disposable_command_buffer() const {
	auto cmd_buffer = device.get_vk_device().allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(command_pool)
		).back();

	cmd_buffer.begin(vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	return cmd_buffer;
}*/

void LowLevelGraphics::create_mesh() {
	{
		auto file = io::File::open("../tools/mesh/chalet.ym");
		auto m_data = MeshData::from_file(file);
		std::cout << m_data.triangles.size() << " triangles loaded" << std::endl;
		static_mesh = StaticMeshInstance(&device, m_data);
	}
	{
		auto file = io::File::open("../tools/image/chalet.jpg.rgba");
		auto image = ImageData::from_file(file);
		std::cout << image.size().x() *image.size().y() << " pixels loaded" << std::endl;
		mesh_texture = Texture(&device, vk::Format::eR8G8B8A8Unorm, image.size(), image.get_raw_data());
	}
}



}

