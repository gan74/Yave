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

#include <yave/descriptors/Binding.h>
#include <yave/descriptors/DescriptorSet.h>

namespace yave {

class Window;

class YaveApp : NonCopyable {

	struct MVP {
		math::Matrix4<> model;
		math::Matrix4<> view;
		math::Matrix4<> proj;
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

		AssetPtr<Material> material;

		Texture mesh_texture;
		core::Vector<StaticMesh> objects;
		TypedBuffer<MVP, BufferUsage::UniformBuffer> uniform_buffer;

		DescriptorSet mvp_set;



};

}

#endif // YAVE_YAVEAPP_H
