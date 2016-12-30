/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef YAVE_YAVEAPP_H
#define YAVE_YAVEAPP_H

#include <yave/yave.h>

#include <yave/Device.h>

#include <yave/material/Material.h>
#include <yave/commands/CmdBufferRecorder.h>
#include <yave/image/Image.h>
#include <yave/Swapchain.h>
#include <yave/objects/StaticMesh.h>
#include <yave/mesh/MeshInstancePool.h>

#include <yave/descriptors/Binding.h>
#include <yave/descriptors/DescriptorSet.h>
#include <yave/scene/SceneView.h>

#include <y/math/Quaternion.h>

namespace yave {

class Window;

class YaveApp : NonCopyable {

	struct MVP {
		math::Matrix4<> view;
		math::Matrix4<> proj;
	};

	struct Offscreen : NonCopyable {
		Offscreen(DevicePtr dptr, const math::Vec2ui& size) :
				depth(dptr, vk::Format::eD32Sfloat, size),
				color(dptr, vk::Format::eR8G8B8A8Unorm, size),
				color2(dptr, vk::Format::eR8G8B8A8Unorm, size),
				pass(dptr, depth.format(), {color.format(), color2.format()}),
				framebuffer(pass, depth, {color, color2}) {

			sem = dptr->vk_device().createSemaphore(vk::SemaphoreCreateInfo());
		}

		~Offscreen() {
			pass.device()->vk_device().destroySemaphore(sem);
		}

		Image<ImageUsage::Texture | ImageUsage::Depth> depth;
		Image<ImageUsage::Texture | ImageUsage::Color> color;
		Image<ImageUsage::Texture | ImageUsage::Color> color2;
		RenderPass pass;
		Framebuffer framebuffer;
		vk::Semaphore sem;
	};

	public:
		YaveApp(DebugParams params);
		~YaveApp();

		void init(Window* window);

		Duration draw();
		void update(math::Vec2 angles = math::Vec2(0));


	private:
		void create_command_buffers();

		void create_assets();

		Instance instance;
		Device device;

		Swapchain* swapchain;

		CmdBufferPool command_pool;
		core::Vector<RecordedCmdBuffer> command_buffers;

		RecordedCmdBuffer offscreen_cmd;

		AssetPtr<Material> material;

		MeshInstancePool mesh_pool;

		Offscreen* offscreen;

		Texture mesh_texture;

		Scene* scene;
		SceneView* scene_view;

		StaticMesh* sq;
		DescriptorSet dummy_ds;

		ComputeShader* compute;
		ComputeProgram* compute_prog;
		TypedBuffer<int, BufferUsage::StorageBuffer | BufferUsage::AttributeBuffer> compute_buffer;
		DescriptorSet compute_ds;

};

}

#endif // YAVE_YAVEAPP_H
