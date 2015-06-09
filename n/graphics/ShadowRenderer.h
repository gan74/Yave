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

#ifndef N_GRAPHICS_SHADOWRENDERER
#define N_GRAPHICS_SHADOWRENDERER

#include "SceneRenderer.h"
#include "BufferedRenderer.h"

namespace n {
namespace graphics {

template<typename T>
class BoxLight;

template<typename T>
class ShadowRenderer : public BufferedRenderer
{
	public:
		ShadowRenderer(uint res) : BufferedRenderer(res) {
			buffer.setAttachmentEnabled(0, false);
			buffer.setDepthEnabled(true);
		}

		const math::Matrix4<T> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<T> &getViewMatrix() const {
			return view;
		}

		Texture getDepth() {
			return buffer.getDepthAttachement();
		}

		math::Matrix4<T> getShadowMatrix() const {
			return proj * view;
		}

	protected:
		math::Matrix4<T> view;
		math::Matrix4<T> proj;
};

class BoxLightShadowRenderer : public ShadowRenderer<float>
{
	public:
		BoxLightShadowRenderer(BoxLight<float> *li, const Scene<> *sc, uint si = 1024);

		virtual ~BoxLightShadowRenderer();

		virtual void *prepare() override;
		virtual void render(void *ptr) override;

	private:
		SceneRenderer *child;
		BoxLight<float> *light;


};

}
}

#endif // N_GRAPHICS_SHADOWRENDERER

