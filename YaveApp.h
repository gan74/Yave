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
#include <yave/images/Image.h>
#include <yave/swapchain/Swapchain.h>
#include <yave/objects/StaticMeshInstance.h>
#include <yave/objects/SkinnedMeshInstance.h>
#include <yave/animations/Animation.h>

#include <yave/bindings/Binding.h>
#include <yave/bindings/DescriptorSet.h>
#include <yave/scene/SceneView.h>

#include <yave/renderers/ColorCorrectionRenderer.h>
#include <yave/renderers/DeferredRenderer.h>
#include <yave/renderers/DepthRenderer.h>
#include <yave/renderers/SceneRenderer.h>
#include <yave/objects/Text.h>


namespace yave {

class Window;

class YaveApp : NonCopyable {
	struct DestroyLaterBase : NonCopyable {
		virtual ~DestroyLaterBase() {
		}
	};

	template<typename T>
	struct DestroyLater : DestroyLaterBase {
		DestroyLater(T&& t) : _obj(std::forward<T>(t)) {
		}

		~DestroyLater() {
		}

		std::remove_reference_t<T> _obj;
	};

	public:
		YaveApp(DebugParams params);
		~YaveApp();

		template<typename... Args>
		void set_swapchain(Args... args) {
			if(swapchain) {
				fatal("Use reset_swapchain.");
			}
			device.vk_queue(QueueFamily::Graphics).waitIdle();
			renderer = nullptr;
			swapchain = new Swapchain(&device, std::forward<Args>(args)...);
			create_renderers();
		}

		void reset_swapchain() {
			device.vk_queue(QueueFamily::Graphics).waitIdle();
			renderer = nullptr;
			swapchain->reset();
			create_renderers();
		}

		void draw();
		void update(math::Vec2 angles = {});


	private:
		void create_assets();
		void create_renderers();

		Instance instance;
		Device device;
		ThreadDevicePtr thread_device;

		core::Vector<core::Unique<DestroyLaterBase>> destroy_laters;

		core::Unique<Swapchain> swapchain;

		Scene* scene;
		SceneView* scene_view;

		Camera camera;
		core::Arc<EndOfPipeline> renderer;


		template<typename T>
		void destroy_later(T&& t) {
			destroy_laters.emplace_back(new DestroyLater<T>(std::forward<T>(t)));
		}

};

}

#endif // YAVE_YAVEAPP_H
