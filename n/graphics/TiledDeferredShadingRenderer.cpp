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
#include "TiledDeferredShadingRenderer.h"


namespace n {
namespace graphics {

Shader<ComputeShader> *createComputeShader() {
	return new Shader<ComputeShader>(
		"\n#define GROUP_X 32\n"
		"\n#define GROUP_Y 30\n"

		"layout(rgba8) uniform writeonly image2D outImage;"
		"layout(local_size_x = GROUP_X, local_size_y = GROUP_Y) in;"
		"const ivec2 groupSize = ivec2(GROUP_X, GROUP_Y);"

		"uniform sampler2D n_0;"
		"uniform sampler2D n_1;"
		"uniform sampler2D n_2;"
		"uniform sampler2D n_D;"

		"uniform mat4 n_InvMatrix;"





		"vec3 unproject(sampler2D depth, vec2 uv) {"
			"vec4 VP = vec4(vec3(uv, texture(depth, uv).x) * 2.0 - 1.0, 1.0);"
			"vec4 P = n_InvMatrix * VP;"
			"return P.xyz / P.w;"
		"}"

		"void main() {"
			"ivec2 coord = ivec2(gl_GlobalInvocationID.xy);"

			"n_GBufferData gbuffer = n_unpackGBuffer(n_0, n_1, n_2, coord);"
			//"vec3 world = unproject(n_D, uv);"

			"imageStore(outImage, coord, vec4(gbuffer.normal, 0));"
		"}"
	);
}


TiledDeferredShadingRenderer::TiledDeferredShadingRenderer(GBufferRenderer *g, const math::Vec2ui &s) : BufferedRenderer(s, true, ImageFormat::RGBA8), gbuffer(g), compute(new ComputeShaderInstance(createComputeShader())) {
	compute->setValue("outImage", getFrameBuffer().getAttachment(0), TextureAccess::WriteOnly);

}

void *TiledDeferredShadingRenderer::prepare() {
	void *ptr = gbuffer->prepare();
	SceneRenderer::FrameData *sc = gbuffer->getSceneRendererData(ptr);
	return new FrameData{
		ptr,
		gbuffer->getRenderer()->getScene()->query<DirectionalLight>(*sc->camera)
	};
}

void TiledDeferredShadingRenderer::render(void *ptr) {
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	gbuffer->render(data->gbufferData);

	compute->setValue("n_0", gbuffer->getFrameBuffer().getAttachment(0));
	compute->setValue("n_1", gbuffer->getFrameBuffer().getAttachment(1));
	compute->setValue("n_2", gbuffer->getFrameBuffer().getAttachment(2));
	compute->setValue("n_D", gbuffer->getFrameBuffer().getDepthAttachment());

	compute->dispatch(math::Vec3ui(getFrameBuffer().getSize() / math::Vec2(32, 30), 1));

	delete data;
}

}
}
