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
	struct FrameData
	{
		math::Matrix4<> proj;
		math::Matrix4<> view;
		RenderQueue queue;
	};

	public:
		struct PerfData
		{
			double time;
			uint objects;
		};

		SceneRenderer(const Scene<> *sc) : BufferableRenderer(), sce(sc), perfData({0, 0}) {
		}

		virtual ~SceneRenderer() {
		}

		PerfData getPerfData() const {
			return perfData;
		}

		virtual void *prepare() override {
			const Camera<> *cam = getCamera();
			if(!cam) {
				fatal("Camera not found");
			}
			return prepare(*cam);
		}

		template<typename T>
		void *prepare(const T &vol) {
			T cam(vol);
			core::Timer timer;
			FrameData *data = new FrameData();
			data->proj = cam.getProjectionMatrix();
			data->view = cam.getViewMatrix();
			core::Array<Renderable *> res = sce->query<Renderable>(cam);
			perfData.time = timer.elapsed();
			perfData.objects = res.size();
			for(Renderable *re : res) {
				re->render(data->queue);
			}
			data->queue.prepare();
			return data;
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
			FrameData *data = reinterpret_cast<FrameData *>(ptr);
			GLContext::getContext()->setProjectionMatrix(data->proj);
			GLContext::getContext()->setViewMatrix(data->view);
			for(const auto q : data->queue.getBatches()) {
				q();
			}
			for(const auto q : data->queue.getFunctions()) {
				q();
			}
			delete data;
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

