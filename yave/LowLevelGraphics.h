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
#ifndef YAVE_LOWLEVELGRAPHICS_H
#define YAVE_LOWLEVELGRAPHICS_H

#include "yave.h"

#include <y/core/Vector.h>
#include <y/math/math.h>
#include <array>

#include "Instance.h"
#include "Device.h"

#include "material/MaterialCompiler.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include <yave/command/CmdBufferRecorder.h>
#include "mesh/Vertex.h"
#include "image/Image.h"
#include "mesh/StaticMeshInstance.h"
#include "Swapchain.h"

namespace yave {

class Window;

struct MVP {
	math::Matrix4<> model;
	math::Matrix4<> view;
	math::Matrix4<> proj;
};

class LowLevelGraphics : NonCopyable {

	Y_TODO(PLEEZ KILL MEEEEEE)


	public:
		LowLevelGraphics(DebugParams params);
		~LowLevelGraphics();

		void init(Window* window);

		void draw();
		void update(math::Vec2 angles = math::Vec2(0));

		vk::Device get_logical_device() const {
			return device.get_vk_device();
		}

		vk::PhysicalDevice get_physical_device() const {
			return device.get_physical_device().get_vk_physical_device();
		}

		vk::Queue get_graphic_queue() const {
			return device.get_vk_queue(QueueFamily::Graphics);
		}

		vk::PhysicalDeviceMemoryProperties get_memory_properties() const {
			return device.get_physical_device().get_vk_memory_properties();
		}

	private:
		void create_descriptor_pool();
		void create_swapchain();
		void create_graphic_pipeline();
		void create_command_buffers();

		void create_mesh();

		void set_surface(Window* window);

		MaterialCompiler* material_compiler;

		Instance instance;
		Device device;


		vk::SurfaceKHR surface;
		Swapchain* swapchain;

		GraphicPipeline pipeline;

		CmdBufferPool command_pool;
		core::Vector<RecordedCmdBuffer> command_buffers;

		Texture mesh_texture;
		StaticMeshInstance static_mesh;
		core::Rc<TypedBuffer<MVP, BufferUsage::UniformBuffer>> uniform_buffer;



};

}

#endif // YAVE_LOWLEVELGRAPHICS_H
