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

#include "DeferredShadingRenderer.h"
#include "DeferredCommon.h"

namespace n {
namespace graphics {

struct LightData
{
	LightData(Light *l) : light(l), shadowData(light->castShadows() ? light->getShadowRenderer()->prepare() : 0) {
	}

	Light *light;
	void *shadowData;
};

struct FrameData
{
	math::Vec3 pos;
	math::Matrix4<> inv;
	core::Array<LightData> lights[LightType::Max];
	void *child;
};










template<LightType Type>
ShaderCombinaison *getShader(const core::String &shadow) {
	static core::Map<core::String, ShaderCombinaison *> shaders[LightType::Max];

	core::String computeDir[LightType::Max] = {"return n_LightPos - pos;",
											   "return n_LightForward;",
											   "return n_LightForward;"};
	core::String attenuate[LightType::Max] = {"float x = min(lightDist, n_LightRadius); "
												  "return sqr(1.0 - sqr(sqr(x / n_LightRadius))) / (sqr(x) + 1.0);",
											  "return 1.0;",
											  "return any(greaterThan(abs(n_LightMatrix * (pos - n_LightPos)), n_LightSize)) ? 0.0 : 1.0;"};
	ShaderCombinaison *shader = shaders[Type].get(shadow, 0);
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"uniform sampler2D n_2;"
			"uniform sampler2D n_D;"
			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"uniform vec3 n_LightPos;"
			"uniform vec3 n_LightForward;"
			"uniform vec3 n_LightColor;"

			"uniform float n_LightRadius;"
			"uniform mat3 n_LightMatrix;"
			"uniform vec3 n_LightSize;"

			"uniform sampler2D n_LightShadow;"
			"uniform float n_LightShadowMul;"
			"uniform mat4 n_LightShadowMatrix;"

			"in vec4 n_ScreenPosition;"

			"out vec4 n_Out;"

			"const float epsilon = 0.0001;"

			+ getBRDFs() +

			"vec3 projectShadow(vec3 p) {"
				"vec4 proj = n_LightShadowMatrix * vec4(p, 1);"
				"return (proj.xyz / proj.w) * 0.5 + 0.5;"
			"}"

			"float computeShadow(vec3 pos) {"
				+ shadow +
			"}"

			"vec3 unproj(vec2 C) {"
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			"vec2 computeTexCoord() {"
				"return (n_ScreenPosition.xy / n_ScreenPosition.w) * 0.5 + 0.5;"
			"}"

			"float attenuate(vec3 lightDir, vec3 pos, float lightDist) {"
				+ attenuate[Type] +
			"}"

			"vec3 computeDir(vec3 pos) {"
				+ computeDir[Type] +
			"}"

			"void main() {"
				"vec2 texCoord = computeTexCoord();"
				"vec4 albedo = texture(n_0, texCoord);"
				"vec3 pos = unproj(texCoord);"
				"vec3 normal = normalize(texture(n_1, texCoord).xyz * 2.0 - 1.0);"
				"vec4 material = texture(n_2, texCoord);"
				"vec3 view = normalize(n_Cam - pos);"
				"vec3 lightDir = computeDir(pos);"
				"float lightDist = length(lightDir);"
				"vec3 lightVec = lightDir / lightDist;"
				"float metallic = material.y;"

				"float shadow = 1.0;"
				"if(n_LightShadowMul != 0.0) {"
					"shadow = computeShadow(pos);"
				"}"

				"float att = attenuate(lightDir, pos, lightDist);"
				"float NoL = saturate(dot(normal, lightDir));"
				"vec3 light = n_LightColor * NoL * att * shadow;"

				"float diffuse = brdf_lambert(lightDir, view, normal, material);"
				"float specular = brdf_cook_torrance(lightDir, view, normal, material);"
				"n_Out = vec4(light * (diffuse * albedo.rgb * (1.0 - metallic) + specular * mix(vec3(1), albedo.rgb, metallic)), albedo.a);"

				//s"n_Out = vec4(shadow);"
			"}"), Type == Directional ? ShaderProgram::NoProjectionShader : ShaderProgram::ProjectionShader);
		shaders[Type][shadow] = shader;
	}
	return shader;
}









template<LightType Type>
void lightGeometryPass(const Light *l, ShaderCombinaison *sh, const math::Vec3 &forward) {
	switch(Type) {
		case Directional:
			GLContext::getContext()->getScreen().draw(getLightMaterial<Type>(), VertexAttribs(), RenderFlag::NoShader);
		break;

		case Box: {
			const BoxLight *box = dynamic_cast<const BoxLight *>(l);
			math::Matrix3<> lightMatrix(forward, -l->getTransform().getY().normalized(), -l->getTransform().getZ().normalized());
			GLContext::getContext()->setModelMatrix(math::Matrix4<>(lightMatrix[0] * box->getSize().x() * -2, 0,
																	lightMatrix[1] * box->getSize().y() * -2, 0,
																	lightMatrix[2] * box->getSize().z() * -2, 0,
																	0, 0, 0, 1).transposed());
			sh->setValue("n_LightSize", box->getSize() * box->getScale());
			sh->setValue("n_LightMatrix", lightMatrix);
			getBox().draw(getLightMaterial<Type>(), VertexAttribs(), RenderFlag::NoShader);
		} break;

		case Point:
			GLContext::getContext()->setModelMatrix(math::Transform<>(l->getPosition(), l->getRadius() + 1).getMatrix());
			getSphere().draw(getLightMaterial<Type>(), VertexAttribs(), RenderFlag::NoShader);
		break;
	}
}



template<LightType Type>
ShaderCombinaison *lightPass(const FrameData *data, GBufferRenderer *child) {
	ShaderCombinaison *shader = 0;
	for(const LightData &ld : data->lights[Type]) {
		const Light *l = ld.light;
		math::Vec3 forward = -l->getTransform().getX().normalized();
		ShaderCombinaison *sh = getShader<Type>(l->castShadows() ? l->getShadowRenderer()->getCompareCode() : "return 1.0;");
		if(sh != shader) {
			shader = sh;
			sh->bind();
			sh->setValue("n_0", child->getFrameBuffer().getAttachement(0));
			sh->setValue("n_1", child->getFrameBuffer().getAttachement(1));
			sh->setValue("n_2", child->getFrameBuffer().getAttachement(2));
			sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
			sh->setValue("n_Inv", data->inv);
			sh->setValue("n_Cam", data->pos);
		}
		sh->setValue("n_LightPos", l->getPosition());
		sh->setValue("n_LightRadius", l->getRadius());
		sh->setValue("n_LightColor", l->getColor().sub(3) * l->getIntensity());
		sh->setValue("n_LightForward", forward);
		sh->setValue("n_LightShadowMul", l->castShadows() ? 1.0 : 0.0);
		if(l->castShadows()) {
			sh->setValue("n_LightShadow", l->getShadowRenderer()->getShadowMap());
			sh->setValue("n_LightShadowMatrix", l->getShadowRenderer()->getShadowMatrix());
		}
		lightGeometryPass<Type>(l, sh, forward);
	}

	return shader;
}










DeferredShadingRenderer::DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s) : BufferedRenderer(s.isNull() ? c->getFrameBuffer().getSize() : s), child(c) {
	buffer.setAttachmentEnabled(0, true);
	buffer.setAttachmentFormat(0, ImageFormat::RGBA16F);
	buffer.setDepthEnabled(true);
}

void *DeferredShadingRenderer::prepare() {
	void *ptr = child->prepare();
	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(ptr);

	return new FrameData({sceneData->camera->getPosition(),
						  (sceneData->proj * sceneData->view).inverse(),
						  {child->getRenderer()->getScene()->query<PointLight>(*sceneData->camera),
						   child->getRenderer()->getScene()->get<DirectionalLight>(),
						   child->getRenderer()->getScene()->get<BoxLight>()},
						  ptr});
}

void DeferredShadingRenderer::render(void *ptr) {
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(data->child);

	child->render(data->child);

	for(uint i = 0; i != LightType::Max; i++) {
		data->lights[i].foreach([](const LightData &l) {
			if(l.light->castShadows()) {
				l.light->getShadowRenderer()->render(l.shadowData);
			}
		});
	}

	GLContext::getContext()->setProjectionMatrix(sceneData->proj);
	GLContext::getContext()->setViewMatrix(sceneData->view);

	{
		buffer.bind();
		buffer.clear(true, false);
		child->getFrameBuffer().blit(FrameBuffer::Depth);

		lightPass<Point>(data, child);

		lightPass<Box>(data, child);

		ShaderCombinaison *sh = lightPass<Directional>(data, child);

		if(sh) {
			sh->unbind();
		}
	}

	delete data;
}

}
}
