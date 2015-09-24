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
			RGBDiffuseRGBNormal
		};

		GBufferRenderer(SceneRenderer *c, const math::Vec2ui &s = math::Vec2ui(0)) : BufferedRenderer(s, true, true, ImageFormat::RGBA16, true), child(c), bufferFormat(RGBDiffuseRGBNormal) {
		}

		virtual ~GBufferRenderer() {
		}

		SceneRenderer *getRenderer() const {
			return child;
		}

		void setFormat(BufferFormat b) {
			if(b != bufferFormat) {
				bufferFormat = b;
				switch(bufferFormat) {
					case RGBDiffuseRGBNormal:
						setBufferFormat(true, true, ImageFormat::RGBA16, true);
					break;
				}
			}
		}

		virtual void *prepare() override {
			return child->prepare();
		}

		virtual SceneRenderer::FrameData *getSceneRendererData(void *d) const {
			return reinterpret_cast<SceneRenderer::FrameData *>(d);
		}

		virtual void render(void *ptr) override {
			getShader()->bindAsDefault();
			getFrameBuffer().bind();
			GLContext::getContext()->auditGLState();
			child->render(ptr);
			ShaderProgram::setDefaultShader((Shader<FragmentShader> *)0);
		}

		static Shader<FragmentShader> *getShader() {
			static Shader<FragmentShader> *sh = new Shader<FragmentShader>(
					"layout(location = 0) out vec4 n_0;"
					"layout(location = 1) out vec4 n_1;"
					"layout(location = 2) out vec4 n_2;"

					"uniform vec4 n_Color;"
					"uniform float n_Metallic;"

					"uniform float n_DiffuseMul;"
					"uniform sampler2D n_DiffuseMap;"

					"uniform float n_NormalMul;"
					"uniform sampler2D n_NormalMap;"

					"uniform float n_RoughnessMul;"
					"uniform sampler2D n_RoughnessMap;"

					"in vec3 n_Position;"
					"in vec3 n_Normal;"
					"in vec3 n_Tangent;"
					"in vec3 n_Binormal;"
					"in vec2 n_TexCoord;"

					"void main() {"
						"vec3 normal = n_Normal;"
						"if(n_NormalMul != 0) {"
							"vec2 normalXY = texture(n_NormalMap, n_TexCoord).xy * 2.0 - 1.0;"
							"vec3 normalMap = vec3(normalXY * n_NormalMul, sqrt(1.0 - dot(normalXY, normalXY)));"
							"mat3 TBN = mat3(normalize(n_Tangent), normalize(n_Binormal), normalize(n_Normal));"
							"normal = TBN * normalMap;"
						"}"
						"float roughness = texture(n_RoughnessMap, n_TexCoord).x * n_RoughnessMul;"
						"vec4 color = n_Color * mix(vec4(1.0), texture(n_DiffuseMap, n_TexCoord), n_DiffuseMul);"
						"n_0 = n_gbuffer0(color, normal, roughness, n_Metallic);"
						"n_1 = n_gbuffer1(color, normal, roughness, n_Metallic);"
						"n_2 = n_gbuffer2(color, normal, roughness, n_Metallic);"
					"}");
			return sh;
		}

	private:
		SceneRenderer *child;
		BufferFormat bufferFormat;

};

}
}

#endif // N_GRAPHICS_GBUFFERRENDERER

