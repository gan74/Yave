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
#include "DeferredCommon.h"

#include <iostream>

namespace n {
namespace graphics {

static constexpr uint MaxLights = 1024;

Shader<ComputeShader> *TiledDeferredShadingRenderer::createComputeShader() {
	return new Shader<ComputeShader>(
		"\n#define GROUP_X 32\n"
		"\n#define GROUP_Y 30\n"
		"\n#define MAX_LIGHTS " + core::String(MaxLights) + "\n"

		"layout(rgba8) uniform writeonly image2D n_Out;"
		"layout(local_size_x = GROUP_X, local_size_y = GROUP_Y) in;"

		"const uint MaxUint = uint(0xFFFFFFFF);"
		"shared uint iMinDepth = MaxUint;"
		"shared uint iMaxDepth = 0;"

		+ DirectionalLightData::toShader() + ""
		+ getBRDFs() +

		"uniform sampler2D n_0;"
		"uniform sampler2D n_1;"
		"uniform sampler2D n_2;"
		"uniform sampler2D n_D;"

		"uniform mat4 n_InvMatrix;"
		"uniform vec3 n_Camera;"




		"uniform n_DirectionalLightBuffer {"
			"n_DirectionalLightData n_DirectionalLights[MAX_LIGHTS];"
		"};"

		"uniform uint n_DirectionalLightCount;"






		"uint float2uint(float f) {"
			"return uint(MaxUint * f);"
		"}"

		"float uint2float(uint i) {"
			"return i / float(MaxUint);"
		"}"

		"vec3 unproject(vec2 uv, float depth) {"
			"vec4 VP = vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);"
			"vec4 P = n_InvMatrix * VP;"
			"return P.xyz / P.w;"
		"}"

		"void main() {"
			"ivec2 coord = ivec2(gl_GlobalInvocationID.xy);"
			"vec2 uv = gl_GlobalInvocationID.xy / vec2(gl_NumWorkGroups.x * gl_WorkGroupSize);"

			"float depth = texelFetch(n_D, coord, 0).x;"

			"uint iDepth = float2uint(depth);"
			"atomicMin(iMinDepth, iDepth);"
			"atomicMax(iMaxDepth, iDepth);"
			"float minDepth = uint2float(iMinDepth);"
			"float maxDepth = uint2float(iMaxDepth);"


			"n_GBufferData gbuffer = n_unpackGBuffer(n_0, n_1, n_2, coord);"
			"vec3 world = unproject(uv, depth);"

			"vec3 worldView = normalize(n_Camera - world);"

			"vec3 diffuseColor = mix(gbuffer.color.rgb, vec3(0.0), gbuffer.metallic);"
			"vec3 specularColor = mix(vec3(0.04), gbuffer.color.rgb, gbuffer.metallic);"


			"vec3 finalColor = vec3(0.0);"
			"for(uint i = 0; i != n_DirectionalLightCount; i++) {"
				"n_DirectionalLightData light = n_DirectionalLights[i];"

				"vec3 diffuse = brdf_lambert(light.forward, worldView, gbuffer.normal, gbuffer.roughness, diffuseColor);"
				"vec3 specular = brdf_cook_torrance(light.forward, worldView, gbuffer.normal, gbuffer.roughness, specularColor);"

				"float NoL = saturate(dot(gbuffer.normal, light.forward));"

				"finalColor += light.color * NoL * (diffuse + specular);"
			"}"





			//"uint threadCount = gl_NumWorkGroups.x * gl_NumWorkGroups.y;"
			//"uint passCount = (n_DirectionalLightCount + threadCount - 1) / threadCount;"

			"barrier();"

			//"imageStore(n_Out, coord, vec4(minDepth, maxDepth, 0, 1));"
			"imageStore(n_Out, coord, vec4(finalColor, 1.0));"
		"}"
	);
}







TiledDeferredShadingRenderer::TiledDeferredShadingRenderer(GBufferRenderer *g, const math::Vec2ui &s) : BufferedRenderer(s, true, ImageFormat::RGBA8), gbuffer(g), compute(new ComputeShaderInstance(createComputeShader())), directionals(MaxLights) {
	compute->setValue("n_Out", getFrameBuffer().getAttachment(0), TextureAccess::WriteOnly);
	compute->setValue("n_0", gbuffer->getFrameBuffer().getAttachment(0));
	compute->setValue("n_1", gbuffer->getFrameBuffer().getAttachment(1));
	compute->setValue("n_2", gbuffer->getFrameBuffer().getAttachment(2));
	compute->setValue("n_D", gbuffer->getFrameBuffer().getDepthAttachment());
	compute->setBuffer("n_DirectionalLightBuffer", directionals);

	logMsg(core::String("Max compute group size = ") + GLContext::getContext()->getHWInt(GLContext::MaxComputeGroupSize));

}

TiledDeferredShadingRenderer::~TiledDeferredShadingRenderer() {
	delete compute;
}

void *TiledDeferredShadingRenderer::prepare() {
	void *ptr = gbuffer->prepare();
	SceneRenderer::FrameData *sceneData = gbuffer->getSceneRendererData(ptr);
	return new FrameData{
		ptr,
		sceneData->camera->getPosition(),
		(sceneData->proj * sceneData->view).inverse(),
		gbuffer->getRenderer()->getScene()->get<DirectionalLight>()
	};
}

void TiledDeferredShadingRenderer::render(void *ptr) {
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	gbuffer->render(data->gbufferData);


	for(uint i = 0; i != data->directionals.size(); i++) {
		DirectionalLight *light = data->directionals[i];
		directionals[i].color = (light->getColor() * light->getIntensity()).sub(3);
		directionals[i].forward = -light->getTransform().getX();
	}
	compute->setValue("n_DirectionalLightCount", data->directionals.size());

	std::cout<<data->directionals.size()<<std::endl;




	compute->setValue("n_InvMatrix", data->inv);
	compute->setValue("n_Camera", data->camPos);
	compute->dispatch(math::Vec3ui(getFrameBuffer().getSize() / math::Vec2(32, 30), 1));

	delete data;
}

}
}
