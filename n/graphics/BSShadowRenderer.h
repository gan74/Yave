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

#ifndef N_GRAPHICS_BSSHADOWRENDERER
#define N_GRAPHICS_BSSHADOWRENDERER

#include "BufferedRenderer.h"
#include "ShadowRenderer.h"
#include "FrameBufferPool.h"
#include "GLContext.h"

namespace n {
namespace graphics {

class BSShadowRenderer : public ShadowRenderer
{
	public:
		BSShadowRenderer(ShadowRenderer *c, uint fHStep = 0, float sharpness = 30) : ShadowRenderer(c->getSize().x(), false, ImageFormat::R32F), child(c), blurs{createBlurShader(false, fHStep ? fHStep : core::log2ui(c->getSize().x())), createBlurShader(true, fHStep ? fHStep : core::log2ui(c->getSize().x()))} {
			mapIndex = 0;
			shaderCode = "vec3 proj = projectShadow(pos);"
						 "float d = texture(n_LightShadow, proj.xy).x;"
						 "float diff = clamp(proj.z - d, 0.0, 1.0);"
						 "return pow(1.0 - diff, " + core::String(sharpness) + ");";
		}

		~BSShadowRenderer() {
			delete blurs[0];
			delete blurs[1];
		}

		virtual void *prepare() override {
			void *ptr = child->prepare();
			proj = child->getProjectionMatrix();
			view = child->getViewMatrix();
			return ptr;
		}

		virtual void render(void *ptr) override {
			child->render(ptr);

			FrameBuffer *temp = GLContext::getContext()->getFrameBufferPool().get(getSize(), false, ImageFormat::RG32F);
			blurs[0]->setValue(ShaderCombinaison::Texture0, child->getShadowMap());
			blurs[1]->setValue(ShaderCombinaison::Texture0, temp->getAttachement(0));

			temp->bind();
			blurs[0]->bind();
			GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

			getFrameBuffer().bind();
			blurs[1]->bind();
			GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

			blurs[1]->unbind();
			GLContext::getContext()->getFrameBufferPool().add(temp);
			child->discardBuffer();
		}

	private:
		ShadowRenderer *child;
		ShaderCombinaison *blurs[2];
};


}
}
#endif // BSSHADOWRENDERER

