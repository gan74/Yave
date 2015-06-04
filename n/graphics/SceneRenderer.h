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

#ifndef N_GRAPHICS_SCENERENDERER
#define N_GRAPHICS_SCENERENDERER

#include <n/utils.h>
#include "BufferableRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "Renderable.h"
#include <n/core/Timer.h>

namespace n {
namespace graphics {

class SceneRenderer : public BufferableRenderer
{
	public:
		struct PerfData
		{
			double time;
			uint objects;
		};

		SceneRenderer(const Scene<> *sc) : BufferableRenderer(), sce(sc), perfData({0, 0}) {
		}

		PerfData getPerfData() const {
			return perfData;
		}

		virtual void *prepare() override {
			const Camera<> *cam = getCamera();
			if(!cam) {
				fatal("Camera not found");
			}

			GLContext::getContext()->setProjectionMatrix(cam->getProjectionMatrix());
			GLContext::getContext()->setViewMatrix(cam->getViewMatrix());

			core::Timer timer;
			core::Array<Renderable *> res = sce->query<Renderable>(*cam);
			perfData.time = timer.elapsed();
			perfData.objects = res.size();
			RenderQueue *queue = new RenderQueue();
			for(Renderable *re : res) {
				re->render(*queue);
			}
			queue->prepare();
			return queue;
		}

		const Camera<> *getCamera() {
			core::Array<Camera<> *> arr = sce->get<Camera<>>();
			if(arr.size() != 1) {
				return 0;
			}
			return arr.first();
		}

		virtual void render(void *ptr) override {
			gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if(!ptr) {
				return;
			}
			RenderQueue *queue = reinterpret_cast<RenderQueue *>(ptr);
			for(const auto q : queue->getBatches()) {
				q();
			}
			for(const auto q : queue->getFunctions()) {
				q();
			}
			delete queue;
		}

		const Scene<> *getScene() const {
			return sce;
		}

	private:
		const Scene<> *sce;
		PerfData perfData;
};


}
}

#endif // N_GRAPHICS_SCENERENDERER

