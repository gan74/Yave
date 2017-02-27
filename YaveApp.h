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
#ifndef YAVE_YAVEAPP_H
#define YAVE_YAVEAPP_H

#include <yave/yave.h>
#include <y/core/Chrono.h>

#include <yave/device/Device.h>

#include <yave/material/Material.h>
#include <yave/commands/CmdBufferRecorder.h>
#include <yave/image/Image.h>
#include <yave/swapchain/Swapchain.h>
#include <yave/objects/StaticMesh.h>
#include <yave/mesh/MeshInstancePool.h>

#include <yave/bindings/Binding.h>
#include <yave/bindings/DescriptorSet.h>
#include <yave/scene/SceneView.h>

#include <yave/renderers/DeferredRenderer.h>
#include <yave/renderers/SceneRenderer.h>
#include <yave/pipeline/Pipeline.h>

namespace yave {

class Window;

class YaveApp : NonCopyable {


	public:
		YaveApp(DebugParams params);
		~YaveApp();

		template<typename... Args>
		void init(Args... args) {
			swapchain = new Swapchain(&device, std::forward<Args>(args)...);
			create_assets();
			create_command_buffers();
		}

		void draw();
		void update(math::Vec2 angles = math::Vec2(0));


	private:
		void create_command_buffers();

		void create_assets();

		Instance instance;
		Device device;

		Swapchain* swapchain;

		CmdBufferPool<> command_pool;

		core::Vector<AssetPtr<Material>> materials;

		MeshInstancePool mesh_pool;
		Texture mesh_texture;

		Scene* scene;
		SceneView* scene_view;

		Camera camera;

		Pipeline* pipeline;

		concurrent::WorkGroup worker;
};

}

#endif // YAVE_YAVEAPP_H
