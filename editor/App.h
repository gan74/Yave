/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef EDITOR_APP_H
#define EDITOR_APP_H

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
#include <yave/bindings/DescriptorArray.h>
#include <yave/scene/SceneView.h>

#include <yave/renderers/TiledDeferredRenderer.h>
#include <yave/renderers/GBufferRenderer.h>
#include <yave/renderers/EndOfPipe.h>

#include <yave/window/Window.h>
#include <renderers/ImGuiRenderer.h>

#include <editor.h>

namespace editor {


class App : NonCopyable {
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
		App(DebugParams params);
		~App();

		template<typename... Args>
		void set_swapchain(Args... args) {
			if(swapchain) {
				fatal("Use reset_swapchain.");
			}
			device.queue(QueueFamily::Graphics).wait();
			renderer = nullptr;
			swapchain = new Swapchain(&device, std::forward<Args>(args)...);
			create_renderers();
		}

		void reset_swapchain() {
			device.queue(QueueFamily::Graphics).wait();
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
		SceneView* shadow_view;

		core::Arc<experimental::Renderer> renderer;

		template<typename T>
		void destroy_later(T&& t) {
			destroy_laters.emplace_back(new DestroyLater<T>(std::forward<T>(t)));
		}


		bool cap_framerate = true;

};

}

#endif // EDITOR_APP_H
