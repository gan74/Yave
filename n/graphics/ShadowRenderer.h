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

class BoxLight;
class SpotLight;

class ShadowRenderer : public Renderer
{
	public:
		ShadowRenderer(uint res) : Renderer(), buffer(0), size(res) {
			shaderCode = "vec3 proj = projectShadow(pos);"
						 "float d = texture(n_LightShadow, proj.xy).x;"
						 "return step(proj.z, d);";
		}

		virtual ~ShadowRenderer() {
			poolBuffer();
		}

		const math::Matrix4<> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<> &getViewMatrix() const {
			return view;
		}

		const math::Vec2ui &getSize() const {
			return size;
		}

		Texture getShadowMap() {
			return buffer->getAttachement(mapIndex);
		}

		math::Matrix4<> getShadowMatrix() const {
			return proj * view;
		}

		const core::String getCompareCode() const {
			return shaderCode;
		}

		void poolBuffer();

		virtual void createBuffer();

	protected:
		uint mapIndex = FrameBuffer::Depth;
		core::String shaderCode;

		FrameBuffer *buffer;
		math::Vec2ui size;
		math::Matrix4<> view;
		math::Matrix4<> proj;
};

class CameraShadowRenderer : public ShadowRenderer
{
	public:
		CameraShadowRenderer(const Scene *sc, uint s);

		virtual ~CameraShadowRenderer();

		virtual void *prepare() override;
		virtual void render(void *ptr) override;

	protected:
		virtual Camera *createCamera() = 0;

	private:
		SceneRenderer *child;


};

class BoxLightShadowRenderer : public CameraShadowRenderer
{
	public:
		BoxLightShadowRenderer(BoxLight *li, const Scene *sc, uint si) : CameraShadowRenderer(sc, si), light(li) {
		}

		virtual ~BoxLightShadowRenderer() {
		}

	protected:
		virtual Camera *createCamera();

	private:
		BoxLight *light;
};


class SpotLightShadowRenderer : public CameraShadowRenderer
{
	public:
		SpotLightShadowRenderer(SpotLight *li, const Scene *sc, uint si) : CameraShadowRenderer(sc, si), light(li) {
		}

		virtual ~SpotLightShadowRenderer() {
		}

	protected:
		virtual Camera *createCamera();

	private:
		SpotLight *light;
};



}
}

#endif // N_GRAPHICS_SHADOWRENDERER

