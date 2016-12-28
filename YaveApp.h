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

		Image<ImageUsageBits::TextureBit | ImageUsageBits::DepthBit> depth;
		Image<ImageUsageBits::TextureBit | ImageUsageBits::ColorBit> color;
		Image<ImageUsageBits::TextureBit | ImageUsageBits::ColorBit> color2;
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
		TypedBuffer<int, BufferUsage::StorageBuffer> compute_buffer;
		DescriptorSet compute_ds;

};

}

#endif // YAVE_YAVEAPP_H
