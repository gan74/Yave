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


namespace yave {

YaveApp::YaveApp(DebugParams params) : instance(params), device(instance), command_pool(&device), mesh_pool(&device), offscreen(nullptr) {
}

void YaveApp::init(Window* window) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMesh));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));

	swapchain = new Swapchain(&device, window);
	offscreen = new Offscreen(&device, swapchain->size());

	create_assets();

	create_command_buffers();
}

YaveApp::~YaveApp() {
	{
		auto map = compute_buffer.map();
		int w = 0;
		for(int i : map) {
			if(i != w++) {
				fatal("Compute error");
			}
		}
		log_msg("Compute = OK");
	}


	delete scene_view;
	delete scene;

	compute_ds = DescriptorSet();
	compute_buffer = decltype(compute_buffer)();
	delete compute_prog;
	delete compute;

	material = nullptr;
	mesh_texture = Texture();

	delete sq;
	delete offscreen;

	delete swapchain;

	command_buffers.clear();
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::create_command_buffers() {
	{

		offscreen_cmd = scene_view->command_buffer(offscreen->framebuffer);
	}

	for(usize i = 0; i != swapchain->buffer_count(); i++) {

		CmdBufferRecorder recorder(command_pool.create_buffer());
		recorder.dispatch(*compute_prog, math::Vec3ui(compute_buffer.size(), 1, 1), compute_ds);

		recorder.bind_framebuffer(swapchain->framebuffer(i));
		recorder.set_viewport(Viewport(swapchain->size()));

		sq->draw(recorder, dummy_ds);

		command_buffers << recorder.end();
	}
}

Duration YaveApp::draw() {
	Chrono ch;

	auto vk_swap = swapchain->vk_swapchain();
	auto image_acquired_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	auto render_finished_semaphore = device.vk_device().createSemaphore(vk::SemaphoreCreateInfo());
	u32 image_index = device.vk_device().acquireNextImageKHR(vk_swap, u64(-1), image_acquired_semaphore, VK_NULL_HANDLE).value;
	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device.vk_queue(QueueFamily::Graphics);

	{
		auto buffer = offscreen_cmd.vk_cmd_buffer();
		auto submit_info = vk::SubmitInfo()
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&image_acquired_semaphore)
				.setPWaitDstStageMask(&pipe_stage_flags)
				.setCommandBufferCount(1)
				.setPCommandBuffers(&buffer)
				.setSignalSemaphoreCount(1)
				.setPSignalSemaphores(&offscreen->sem);

		graphic_queue.submit(1, &submit_info, VK_NULL_HANDLE);
	}
	{

		auto buffer = command_buffers[image_index].vk_cmd_buffer();
		auto submit_info = vk::SubmitInfo()
				.setWaitSemaphoreCount(1)
				.setPWaitSemaphores(&offscreen->sem)
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
	/*const_cast<StaticMesh&>(scene->static_meshes().first()).transform() =
				math::rotation(angles.x(), math::Vec3(0, 0, 1)) *
				math::rotation(angles.y() + math::pi<float> * 0.5f, math::Vec3(0, 1, 0));*/

	/*auto rot = math::rotation(angles.x(), math::Vec3(0, 0, 1)) * math::rotation(angles.y(), math::Vec3(0, 1, 0));

	auto mapping = uniform_buffer.map();
	auto& mvp = *mapping.begin();

	mvp.view = math::look_at(math::Vec3((rot * math::vec(5., 0., 0., 1.)).sub(3)), math::Vec3());*/

	auto cam_pos =
			(math::rotation(angles.x(), math::Vec3(0, 0, 1)) *
			math::rotation(angles.y() + math::pi<float> * 0.5f, math::Vec3(0, 1, 0))) * math::Vec4(2.5, 0, 0, 1);

	scene_view->set_view(math::look_at(cam_pos.sub(3) / cam_pos.w(), math::Vec3()));

	/*for(const auto& m : scene->static_meshes()) {
		std::cout << m.position().x() << ", " << m.position().y() << ", " << m.position().z() << std::endl;
	}*/
}

void YaveApp::create_assets() {
	using Vec3 = math::Vec3;
	using Vec2 = math::Vec2;

	auto plan = AssetPtr<StaticMeshInstance>(mesh_pool.create_static_mesh(MeshData{
		{{Vec3(-1, -1, 0), Vec3(0, 0, 1), Vec2(0, 0)},
		 {Vec3(-1, 1, 0), Vec3(0, 0, 1), Vec2(0, 1)},
		 {Vec3(1, 1, 0), Vec3(0, 0, 1), Vec2(1, 1)},
		 {Vec3(1, -1, 0), Vec3(0, 0, 1), Vec2(1, 0)}
		},
		{{{0, 2, 1}}, {{0, 3, 2}}}}));

	{
		{
			auto file = io::File::open("../tools/image/chalet.jpg.rgba");
			auto image = ImageData::from_file(file);
			log_msg(core::String() + (image.size().x() * image.size().y()) + " pixels loaded");
			mesh_texture = Texture(&device, vk::Format::eR8G8B8A8Unorm, image.size(), image.raw_data());
		}
		material = asset_ptr(Material(&device, MaterialData()
				.set_frag_data(SpirVData::from_file(io::File::open("basic.frag.spv")))
				.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv")))
				.set_bindings(core::vector(Binding(TextureView(mesh_texture))))
			));
	}


	core::Vector<const char*> meshes = {"../tools/mesh/chalet.ym"/*, "../tools/mesh/cube.ym"*/};
	core::Vector<StaticMesh> objects;
	for(auto name : meshes) {
		auto m_data = MeshData::from_file(io::File::open(name));
		log_msg(core::str() + m_data.triangles.size() + " triangles loaded");
		auto mesh = AssetPtr<StaticMeshInstance>(mesh_pool.create_static_mesh(m_data));
		for(usize i = 0; i != 10; i++) {
			{
				auto m = StaticMesh(mesh, material);
				m.set_position(math::Vec3(float(i) - 5, 0, 0));

				objects << std::move(m);
			}
		}
	}
	scene = new Scene(std::move(objects));
	scene_view = new SceneView(&device, *scene);

	{
		auto sq_mat = asset_ptr(Material(&device, MaterialData()
				.set_frag_data(SpirVData::from_file(io::File::open("light.frag.spv")))
				.set_vert_data(SpirVData::from_file(io::File::open("sq.vert.spv")))
				.set_bindings({TextureView(offscreen->color), TextureView(offscreen->color2), TextureView(offscreen->depth)})
			));
		sq = new StaticMesh(plan, sq_mat);
	}

	int p = 0;
	for(StaticMesh& m : objects) {
		float x = p += 1;
		m.set_position(Vec3(p % 2 ? x : -x, 0.f, 0.f));
	}

	{
		compute = new ComputeShader(&device, SpirVData::from_file(io::File::open("comp.comp.spv")));
		compute_prog = new ComputeProgram(*compute);
		compute_buffer = decltype(compute_buffer)(&device, 1000);
		auto map = compute_buffer.map();
		std::fill(map.begin(), map.end(), 0);

		compute_ds = DescriptorSet(&device, {Binding(compute_buffer)});
	}


	dummy_ds = DescriptorSet(&device, {});
}



}

