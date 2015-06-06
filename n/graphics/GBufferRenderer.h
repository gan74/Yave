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
			setFormat(RGBDiffuseRGBNormal);
		}

		~GBufferRenderer() {
		}

		SceneRenderer *getRenderer() const {
			return child;
		}

		void setFormat(BufferFormat) {
			buffer.setAttachmentEnabled(0, true);
			buffer.setAttachmentEnabled(1, true);
			buffer.setAttachmentFormat(1, ImageFormat::RGBA16);
			buffer.setAttachmentEnabled(2, true);
			buffer.setDepthEnabled(true);
		}

		virtual void *prepare() override {
			return child->prepare();
		}

		virtual void render(void *ptr) override {
			getShader()->bindAsDefault();
			buffer.bind();
			child->render(ptr);
			ShaderProgram::setDefaultShader((Shader<FragmentShader> *)0);
		}

	private:
		static Shader<FragmentShader> *getShader() {
			static Shader<FragmentShader> *sh = new Shader<FragmentShader>(
					"layout(location = 0) out vec4 n_0;"
					"layout(location = 1) out vec4 n_1;"
					"layout(location = 2) out vec4 n_2;"

					"uniform vec4 n_Color;"
					"uniform float n_Roughness;"
					"uniform float n_Metallic;"

					"uniform float n_DiffuseMul;"
					"uniform sampler2D n_DiffuseMap;"

					"uniform float n_NormalMul;"
					"uniform sampler2D n_NormalMap;"

					"in vec3 n_Position;"
					"in vec3 n_Normal;"
					"in vec3 n_Tangent;"
					"in vec3 n_Binormal;"
					"in vec2 n_TexCoord;"

					"void main() {"
						"vec3 normal = n_Normal;"
						"if(n_NormalMul != 0) {"
							"vec2 normalXY = texture(n_NormalMap, n_TexCoord).xy * 2.0 - 1.0;"
							"vec3 normalMap = vec3(normalXY, sqrt(1.0 - dot(normalXY, normalXY)));"
							"mat3 TBN = mat3(normalize(n_Tangent), normalize(n_Binormal), normalize(n_Normal));"
							"normal = TBN * normalMap;"
						"}"
						"vec4 color = n_Color * mix(vec4(1.0), texture(n_DiffuseMap, n_TexCoord), n_DiffuseMul);"
						"n_0 = n_gbuffer0(color, normal, n_Roughness, n_Metallic);"
						"n_1 = n_gbuffer1(color, normal, n_Roughness, n_Metallic);"
						"n_2 = n_gbuffer2(color, normal, n_Roughness, n_Metallic);"
					"}");
			return sh;
		}

		SceneRenderer *child;

};

}
}

#endif // N_GRAPHICS_GBUFFERRENDERER

