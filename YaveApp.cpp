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

YaveApp::YaveApp(DebugParams params) : instance(params), device(instance), mesh_pool(&device) {
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMesh));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));
}

YaveApp::~YaveApp() {
	device.vk_queue(QueueFamily::Graphics).waitIdle();

	delete scene_view;
	delete scene;

	material = decltype(material)();
	mesh_texture = Texture();

	delete swapchain;
}

vk::Extent2D extent(const math::Vec2ui& v) {
	return vk::Extent2D{v.x(), v.y()};
}

void YaveApp::draw() {
	core::DebugTimer f("frame", core::Duration::milliseconds(8));

	FrameToken frame = swapchain->next_frame();

	CmdBufferRecorder<> recorder(device.create_cmd_buffer());
	renderer->process(frame, recorder);
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
	float dist = 3.0f;

	auto cam_tr = math::rotation(angles.x(), {0, 0, -1}) * math::rotation(angles.y(), {0, 1, 0});
	auto cam_pos = cam_tr * math::Vec4(dist, 0, 0, 1);
	auto cam_up = cam_tr * math::Vec4(0, 0, 1, 0);

	camera.set_view(math::look_at(cam_pos.to<3>() / cam_pos.w(), math::Vec3(), cam_up.to<3>()));
	camera.set_proj(math::perspective(math::to_rad(45), 4.0f / 3.0f, 0.01f,  10000.0f));
}

void YaveApp::create_assets() {
	{
		{
			auto image = ImageData::from_file(io::File::open("../tools/image_to_yt/chalet.jpg.yt").expected("Unable to load texture file."));
			mesh_texture = Texture(&device, image);
		}
		material = AssetPtr<Material>(Material(&device, MaterialData()
				.set_frag_data(SpirVData::from_file(io::File::open("textured.frag.spv").expected("Unable to load spirv file")))
				.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file")))
				.set_bindings({Binding(TextureView(mesh_texture))})
			));
	}


	core::Vector<const char*> meshes = {"../tools/obj_to_ym/chalet.obj.ym", "../tools/obj_to_ym/small_cube.obj.ym"};
	core::Vector<core::Unique<StaticMesh>> objects;
	core::Vector<core::Unique<Renderable>> renderables;

	{
		auto image = ImageData::from_file(io::File::open("../tools/image_to_yt/height.png.yt").expected("Unable to load texture file."));
		renderables << new HeightmapTerrain(mesh_pool, AssetPtr<Texture>(Texture(&device, image)));
	}


	for(auto name : meshes) {
		auto m_data = MeshData::from_file(io::File::open(name).expected("Unable to load mesh file"));
		log_msg(core::str() + m_data.triangles.size() + " triangles loaded");
		auto mesh = AssetPtr<StaticMeshInstance>(std::move(mesh_pool.create_static_mesh(m_data).expected("Unable to allocate static mesh")));

		int size = 0;
		for(int x = -size; x != size + 1; ++x) {
			for(int y = -size; y != size + 1; ++y) {
				auto obj = StaticMesh(mesh, material);
				obj.set_position(math::Vec3{x, y, 0} * 5);
				objects << std::move(obj);
			}
		}
	}


	scene = new Scene(std::move(objects), std::move(renderables));
	scene_view = new SceneView(*scene, camera);

	update();

	{
		auto culling = core::Rc<CullingNode>(new CullingNode(*scene_view));
		auto gbuffer = core::Rc<GBufferRenderer>(new GBufferRenderer(&device, swapchain->size(), culling));
		auto deferred = core::Rc<BufferRenderer>(new DeferredRenderer(gbuffer));
		renderer = core::Rc<EndOfPipeline>(new ColorCorrectionRenderer(deferred));
	}

	//renderer = new DeferredRenderer(*scene_view, swapchain->size());

}

}

