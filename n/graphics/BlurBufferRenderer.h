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

#ifndef N_GRAPHICS_BLURBUFFERRENDERER_H
#define N_GRAPHICS_BLURBUFFERRENDERER_H

#include "BufferedRenderer.h"
#include "FrameBufferPool.h"

namespace n {
namespace graphics {

class BlurBufferRenderer : public BufferedRenderer
{
	public:
		static ShaderCombinaison *createBlurShader(bool vertical, uint hSteps, float var = 2.0) {
			double mul = 1.0 / sqrt(2.0 * math::pi * var * var);
			double tot = 0.0;
			core::String a = vertical ? "0, " : "";
			core::String b = vertical ? "" : ", 0";
			core::String acc;
			for(int i = -int(hSteps); i != int(hSteps + 1); i++) {
				double w = mul * exp(-(i * i / (2 * var * var)));
				tot += w;
				acc = acc + "acc += " + w + " * textureOffset(n_0, n_TexCoord, ivec2(" + a + i + b + "));";
			}
			return new ShaderCombinaison(new Shader<FragmentShader>(
				"uniform sampler2D n_0;"
				"out vec4 n_Out;"
				"in vec2 n_TexCoord;"
				"void main() {"
					"vec4 acc;"
					+ acc +
					"n_Out = acc / " + tot + ";"
				"}"), ShaderProgram::NoProjectionShader);
		}

		BlurBufferRenderer(BufferedRenderer *c, uint hSteps = 5, float var = 2.0) : BufferedRenderer(c->getFrameBuffer().getSize()), child(c), shaders{createBlurShader(false, hSteps, var), createBlurShader(true, hSteps, var)} {
			buffer.setAttachmentEnabled(FrameBuffer::Depth, false);
			buffer.setAttachmentEnabled(0, true);
			buffer.setAttachmentFormat(0, child->getFrameBuffer().getAttachement(0).getFormat());

			shaders[0]->setValue("n_0", child->getFrameBuffer().getAttachement(0));
		}

		~BlurBufferRenderer() {
			delete shaders[0];
			delete shaders[1];
		}

		virtual void *prepare() override {
			return child->prepare();
		}

		virtual void render(void *ptr) override {
			child->render(ptr);

			FrameBuffer *temp = GLContext::getContext()->getFrameBufferPool().get(buffer.getSize(), false, child->getFrameBuffer().getAttachement(0).getFormat());
			shaders[1]->setValue("n_0", temp->getAttachement(0));

			shaders[0]->bind();
			temp->bind();
			GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

			shaders[1]->bind();
			buffer.bind();
			GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

			shaders[1]->unbind();
			GLContext::getContext()->getFrameBufferPool().add(temp);
		}

	private:
		BufferedRenderer *child;
		ShaderCombinaison *shaders[2];
};

}
}

#endif // N_GRAPHICS_BLURBUFFERRENDERER_H
