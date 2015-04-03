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

#ifndef N_GRAPHICS_GBUFFERRENDERER
#define N_GRAPHICS_GBUFFERRENDERER

#include "BufferedRenderer.h"
#include "SceneRenderer.h"
#include "ShaderCombinaison.h"
#include "Shader.h"

namespace n {
namespace graphics {


class GBufferRenderer : public BufferedRenderer
{

	public:
		enum BufferFormat
		{
			RGBDiffuseRGBNormal = 0
		};

		GBufferRenderer(SceneRenderer *c, const math::Vec2ui &s = math::Vec2ui(0)) : BufferedRenderer(s), child(c) {
			shader = new ShaderCombinaison(createShader());
			setFormat(RGBDiffuseRGBNormal);
		}

		~GBufferRenderer() {
			delete shader;
		}

		SceneRenderer *getRenderer() const {
			return child;
		}

		void setFormat(BufferFormat) {
			buffer.setAttachmentEnabled(0, true);
			buffer.setAttachmentEnabled(1, true);
			buffer.setDepthEnabled(true);
		}

		virtual void *prepare() override {
			return child->prepare();
		}

		virtual void render(void *ptr) override {
			shader->bind();
			buffer.bind();
			child->render(ptr);
		}

	private:
		static Shader<FragmentShader> *createShader() {
			static Shader<FragmentShader> *sh = new Shader<FragmentShader>(
					"layout(location = 0) out vec4 n_0;"
					"layout(location = 1) out vec4 n_1;"

					"uniform vec4 n_Color;"
					"uniform float n_Roughness;"
					"uniform float n_Metallic;"
					"uniform float n_DiffuseMul;"
					"uniform sampler2D n_Diffuse;"

					"in vec3 n_Position;"
					"in vec3 n_Normal;"
					"in vec2 n_TexCoord;"

					"void main() {"
						"vec4 color = n_Color * mix(vec4(1.0), texture(n_Diffuse, n_TexCoord), n_DiffuseMul);"
						"n_0 = n_gbuffer0(color, n_Normal, n_Roughness, n_Metallic);"
						"n_1 = n_gbuffer1(color, n_Normal, n_Roughness, n_Metallic);"
					"}");
					std::cout<<sh->getLogs()<<std::endl;
			return sh;
		}

		SceneRenderer *child;
		ShaderCombinaison *shader;

};

}
}

#endif // N_GRAPHICS_GBUFFERRENDERER

