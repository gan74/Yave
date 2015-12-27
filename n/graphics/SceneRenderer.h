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
		struct FrameData
		{
			math::Matrix4<> proj;
			math::Matrix4<> view;
			const Camera *camera;
			RenderQueue queue;
		};

		SceneRenderer(const Scene *sc) : BufferableRenderer(), sce(sc) {
		}

		virtual ~SceneRenderer() {
		}

		virtual void *prepare() override {
			return prepare(RenderFlag::None);
		}

		const Camera *getCamera() {
			core::Array<Camera *> arr = sce->get<Camera>();
			if(arr.isEmpty()) {
				return 0;
			}
			return arr.first();
		}

		virtual void render(void *ptr) override {
			render(ptr, RenderFlag::None);
		}

		const Scene *getScene() const {
			return sce;
		}

		void *prepare(RenderFlag renderFlags) {
			const Camera *cam = getCamera();
			if(!cam) {
				fatal("Camera not found");
			}
			return prepare(cam, renderFlags);
		}


		void *prepare(const Camera *cam, RenderFlag renderFlags = RenderFlag::None) {
			N_LOG_PERF;
			FrameData *data = new FrameData();
			data->proj = cam->getProjectionMatrix();
			data->view = cam->getViewMatrix();
			data->camera = cam;
			core::Array<Renderable *> res = sce->query<Renderable>(*cam);
			for(Renderable *re : res) {
				re->render(data->queue, renderFlags);
			}
			data->queue.prepare();
			return data;
		}

		void render(void *ptr, RenderFlag renderFlags) {
			if(!ptr) {
				return;
			}
			N_LOG_PERF;
			if(!(renderFlags & RenderFlag::Overlay)) {
				FrameBuffer::clear(true, true);
			}
			FrameData *data = reinterpret_cast<FrameData *>(ptr);
			GLContext::getContext()->setProjectionMatrix(data->proj);
			GLContext::getContext()->setViewMatrix(data->view);
			data->queue.present(renderFlags);
			delete data;
		}

	private:
		const Scene *sce;
};

}
}

#endif // N_GRAPHICS_SCENERENDERER

