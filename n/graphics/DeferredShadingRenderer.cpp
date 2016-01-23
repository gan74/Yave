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

#include "ImageLoader.h"

namespace n {
namespace graphics {

template<typename T>
constexpr LightType getLightType() {
	return LightType::Max;
}

template<>
constexpr LightType getLightType<PointLight>() {
	return LightType::Point;
}

template<>
constexpr LightType getLightType<SpotLight>() {
	return LightType::Spot;
}

template<>
constexpr LightType getLightType<DirectionalLight>() {
	return LightType::Directional;
}

template<>
constexpr LightType getLightType<BoxLight>() {
	return LightType::Box;
}







struct LightData
{
	template<typename T>
	LightData(T *l) : light(l), shadowData(l->getShadowRenderer() ? l->getShadowRenderer()->prepare() : 0) {
		if((void *)((Light *)l) != (void *)l) {
			fatal("Virtual inherited Light");
		}
	}

	template<typename T>
	T *to() {
		return reinterpret_cast<T *>(light);
	}

	template<typename T>
	const T *to() const {
		return reinterpret_cast<const T *>(light);
	}


	void *light;
	void *shadowData;
};

struct DeferredShadingRenderer::FrameData
{
	math::Vec3 pos;
	math::Matrix4<> inv;
	core::Array<LightData> lights[LightType::Max];
	void *child;

	void renderShadows() {
		N_LOG_PERF;
		for(uint i = 0; i != LightType::Max; i++) {
			lights[i].foreach([](const LightData &l) {
				if(l.to<Light>()->getShadowRenderer()) {
					l.to<Light>()->getShadowRenderer()->render(l.shadowData);
				}
			});
		}
	}

	void discardShadows() {
		for(uint i = 0; i != LightType::Max; i++) {
			lights[i].foreach([](const LightData &l) {
				if(l.to<Light>()->getShadowRenderer()) {
					l.to<Light>()->getShadowRenderer()->discardBuffer();
				}
			});
		}
	}
};






template<LightType Type>
ShaderInstance *getShader(const core::String &shadow, bool gBufferShadows = false, DeferredShadingRenderer::LightingDebugMode debug = DeferredShadingRenderer::None) {
	static core::Map<core::String, ShaderInstance *> shaders[LightType::Max][2][DeferredShadingRenderer::Max];

	static core::String debugStrs[] = {"", "n_Out = vec4(vec3(att), 1.0);", "n_Out = vec4(vec3(shadow), 1.0);"};
	static core::String computeDir[LightType::Max] = { "return n_LightPos - pos;",
													   "return n_LightPos - pos;",
													   "return n_LightForward;",
													   "return n_LightForward;"};
	static core::String attenuate[LightType::Max] = {	"float x = min(lightDist, n_LightRadius); "
															"return sqr(1.0 - sqr(sqr(x / n_LightRadius))) / (sqr(x) + 1.0);",
														"float x = min(lightDist, n_LightRadius); "
															  "float falloff = sqr(1.0 - sqr(sqr(x / n_LightRadius))) / (sqr(x) + 1.0);"
															  "float spotFalloff = pow((dot(lightDir, n_LightForward) - n_LightCosCutOff) / (1.0 - n_LightCosCutOff), n_LightExponent);"
															  "return max(0.0, falloff * spotFalloff);",
														"return 1.0;",
														"return any(greaterThan(abs(n_LightMatrix * (pos - n_LightPos)), n_LightSize)) ? 0.0 : 1.0;"};
	ShaderInstance *shader = shaders[Type][gBufferShadows][debug].get(shadow, 0);
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
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
			"uniform float n_LightScale;"
			"uniform float n_LightCosCutOff;"
			"uniform float n_LightExponent;"
			"uniform mat3 n_LightMatrix;"
			"uniform vec3 n_LightSize;"

			"uniform sampler2D n_LightShadow;"
			"uniform float n_LightShadowMul;"
			"uniform mat4 n_LightShadowMatrix;"

			"in vec4 n_ScreenPosition;"

			"out vec4 n_Out;"

			+ getBRDFs() +

			"vec3 projectShadow(vec3 p) {"
				"vec4 proj = n_LightShadowMatrix * vec4(p, 1.0);"
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

			"float test_brdf(vec3 L, vec3 V, vec3 N, vec4 M) {"
				"vec3 R = reflect(-V, N);"
				"return max(0.0, dot(L, R));"
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
				"lightDir /= lightDist;"
				"lightDist /= n_LightScale;"
				"float metallic = material.y;"

				"float shadow = 1.0;"
				"if(n_LightShadowMul != 0.0) {"
					"shadow = n_LightShadowMul * computeShadow(pos);"
				"}"
				+ (gBufferShadows ? "shadow *= material.w;" : "") +

				"float att = attenuate(lightDir, pos, lightDist);"
				"float NoL = saturate(dot(normal, lightDir));"
				"float roughness = material.x + epsilon;"
				"vec3 light = n_LightColor * NoL * att * shadow;"

				"vec3 diffuseColor = mix(albedo.rgb, vec3(0.0), metallic);"
				"vec3 specularColor = mix(vec3(0.04), albedo.rgb, metallic);"

				"vec3 diffuse = brdf_lambert(lightDir, view, normal, roughness, diffuseColor);"
				"vec3 specular = brdf_cook_torrance(lightDir, view, normal, roughness, specularColor);"

				"n_Out = vec4(light * (diffuse + specular), albedo.a);"

				+ debugStrs[debug] +

			"}"), Type == Directional ? ShaderProgram::NoProjectionShader : ShaderProgram::ProjectionShader);
		shaders[Type][gBufferShadows][debug][shadow] = shader;
	}
	return shader;
}






#warning box and spot lights shadow matrix are broken

void lightGeometryPass(const DirectionalLight *, ShaderInstance *, const math::Vec3 &) {
	GLContext::getContext()->getScreen().draw(getLightMaterial<Directional>());
}

void lightGeometryPass(const BoxLight *l, ShaderInstance *sh, const math::Vec3 &forward) {
	math::Matrix3<> lightMatrix(forward, -l->getTransform().getY().normalized(), -l->getTransform().getZ().normalized());
	GLContext::getContext()->setModelMatrix(math::Matrix4<>(lightMatrix[0] * l->getSize().x() * -2, 0,
															lightMatrix[1] * l->getSize().y() * -2, 0,
															lightMatrix[2] * l->getSize().z() * -2, 0,
															0, 0, 0, 1).transposed());
	sh->setValue("n_LightSize", l->getSize() * l->getScale());
	sh->setValue("n_LightMatrix", lightMatrix);
	getBox().draw(getLightMaterial<Box>());
}

void lightGeometryPass(const PointLight *l, ShaderInstance *, const math::Vec3 &) {
	GLContext::getContext()->setModelMatrix(math::Transform<>(l->getPosition(), l->getRadius() + 1).getMatrix());
	getSphere().draw(getLightMaterial<Point>());
}

void lightGeometryPass(const SpotLight *l, ShaderInstance *sh, const math::Vec3 &) {
	GLContext::getContext()->setModelMatrix(math::Transform<>(l->getPosition(), l->getRadius() + 1).getMatrix());
	sh->setValue("n_LightCosCutOff", cos(l->getCutOff() * 0.5));
	sh->setValue("n_LightExponent", l->getExponent());
	getSphere().draw(getLightMaterial<Spot>());
}

template<typename T>
void setupShadows(const T *l, ShaderInstance *shader) {
	if(l->getShadowRenderer()) {
		shader->setValue("n_LightShadow", l->getShadowRenderer()->getShadowMap());
		shader->setValue("n_LightShadowMatrix", l->getShadowRenderer()->getShadowMatrix());
	}
	shader->setValue("n_LightShadowMul", l->castShadows() ? 1.0 : 0.0);
}

template<typename T>
const core::String getShadowCode(const T *l) {
	return l->getShadowRenderer() ? l->getShadowRenderer()->getCompareCode() : "return 1.0;";
}

template<typename T>
void renderOneShadow(const T *l, void *data) {
	N_LOG_PERF;
	l->getShadowRenderer()->render(data);
}

template<typename T>
ShaderInstance *lightPass(const DeferredShadingRenderer::FrameData *data, DeferredShadingRenderer *renderer) {
	constexpr LightType Type = getLightType<T>();
	ShaderInstance *shader = 0;
	for(const LightData &ld : data->lights[Type]) {
		const T *l = ld.to<T>();
		math::Vec3 forward = -l->getTransform().getX().normalized();
		if(l->getShadowRenderer() && renderer->shadowMode == DeferredShadingRenderer::Memory) {
			renderOneShadow(l, ld.shadowData);
		}
		ShaderInstance *sh = getShader<Type>(getShadowCode(l), l->castGBufferShadows(), renderer->debugMode);
		if(sh != shader) {
			shader = sh;
			shader->bind();
			shader->setValue(SVTexture0, renderer->child->getFrameBuffer().getAttachment(0));
			shader->setValue(SVTexture1, renderer->child->getFrameBuffer().getAttachment(1));
			shader->setValue(SVTexture2, renderer->child->getFrameBuffer().getAttachment(2));
			shader->setValue("n_D", renderer->child->getFrameBuffer().getDepthAttachment());
			shader->setValue("n_Inv", data->inv);
			shader->setValue("n_Cam", data->pos);
		}

		shader->setValue("n_LightPos", l->getPosition());
		shader->setValue("n_LightScale", l->getScale());
		shader->setValue("n_LightRadius", l->getRadius() / l->getScale());
		shader->setValue("n_LightColor", l->getColor().sub(3) * l->getIntensity());
		shader->setValue("n_LightForward", forward);
		setupShadows(l, shader);
		renderer->getFrameBuffer().bind();
		lightGeometryPass(l, shader, forward);
		if(l->getShadowRenderer() && renderer->shadowMode == DeferredShadingRenderer::Memory) {
			l->getShadowRenderer()->discardBuffer();
		}
	}
	return shader;
}

ShaderInstance *rnn(ShaderInstance *a, ShaderInstance *b) {
	return b ? b : a;
}










DeferredShadingRenderer::DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s) : BufferedRenderer(s, true, ImageFormat::RGBA16F), child(c), debugMode(None), shadowMode(Memory) {
}

void *DeferredShadingRenderer::prepare() {
	void *ptr = child->prepare();
	N_LOG_PERF;
	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(ptr);

	return new FrameData({sceneData->camera->getPosition(),
						  (sceneData->proj * sceneData->view).inverse(),
						  {child->getRenderer()->getScene()->query<PointLight>(*sceneData->camera),
						   child->getRenderer()->getScene()->query<SpotLight>(*sceneData->camera),
						   child->getRenderer()->getScene()->get<DirectionalLight>(),
						   child->getRenderer()->getScene()->get<BoxLight>()},
						  ptr});
}

void DeferredShadingRenderer::render(void *ptr) {
	N_LOG_PERF;
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(data->child);

	if(shadowMode == DrawCalls) {
		data->renderShadows();
	}

	child->render(data->child);

	GLContext::getContext()->setProjectionMatrix(sceneData->proj);
	GLContext::getContext()->setViewMatrix(sceneData->view);

	getFrameBuffer().bind();
	getFrameBuffer().clear(true, false);
	child->getFrameBuffer().blit(FrameBuffer::Depth);

	ShaderInstance *sh = 0;

	sh = rnn(sh, lightPass<PointLight>(data, this));

	sh = rnn(sh, lightPass<SpotLight>(data, this));

	sh = rnn(sh, lightPass<BoxLight>(data, this));

	sh = rnn(sh, lightPass<DirectionalLight>(data, this));

	if(sh) {
		sh->unbind();
	}

	if(shadowMode == DrawCalls) {
		data->discardShadows();
	}

	delete data;
}


}
}
