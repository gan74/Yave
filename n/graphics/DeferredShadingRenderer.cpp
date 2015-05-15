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
#include "Light.h"
#include "VertexArrayObject.h"
#include "GL.h"

#include "ImageLoader.h"
#include "CubeMap.h"

namespace n {
namespace graphics {

const Material<float> &getLightMaterial() {
	static Material<> mat;
	if(mat.isNull()) {
		internal::Material<> i;
		i.blend = Add;
		i.depthTested = false;
		mat = Material<>(i);
	}
	return mat;
}

const Material<float> &getCompositionMaterial() {
	static Material<> mat;
	if(mat.isNull()) {
		internal::Material<> i;
		i.depthTested = false;
		mat = Material<>(i);
	}
	return mat;
}

const CubeMap &getCube() {
	static CubeMap *c = 0;
	if(!c) {
		c = new CubeMap(Texture(ImageLoader::load<core::String>("skybox/top.tga")),
						Texture(ImageLoader::load<core::String>("skybox/bottom.tga")),
						Texture(ImageLoader::load<core::String>("skybox/right.tga")),
						Texture(ImageLoader::load<core::String>("skybox/left.tga")),
						Texture(ImageLoader::load<core::String>("skybox/front.tga")),
						Texture(ImageLoader::load<core::String>("skybox/back.tga")));
	}
	return *c;
}

enum LightType
{
	Point,
	Directional,

	Max
};

struct FrameData
{
	Camera<> *cam;
	core::Array<Light<> *> lights[LightType::Max];
	void *child;
};

ShaderCombinaison *getShader(LightType type) {
	static ShaderCombinaison *shader[LightType::Max] = {0};
	core::String computeDir[LightType::Max] = {"return n_LightPos - x;", "return n_LightPos;"};
	core::String attenuate[LightType::Max] = {"float a = 1.0 - x / n_LightRadius; return a * a * a;", "return 1.0;"};
	if(!shader[type]) {
		shader[type] = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_1;"
			"uniform sampler2D n_2;"
			"uniform sampler2D n_D;"
			"uniform samplerCube n_Cube;"
			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"uniform vec3 n_LightPos;"
			"uniform vec3 n_LightColor;"
			"uniform float n_LightRadius;"

			"in vec2 n_TexCoord;"
			"in vec4 n_Position;"

			"out vec4 n_Out;"

			"vec3 unproj(vec2 C) {"
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			"vec3 unproj() {"
				"return unproj((n_Position.xy / n_Position.w) * 0.5 + 0.5);"
			"}"

			"float attenuate(float x) {"
				+ attenuate[type] +
			"}"

			"vec3 computeDir(vec3 x) {"
				+ computeDir[type] +
			"}"

			"void main() {"
				"vec3 pos = unproj();"
				"vec3 normal = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"vec3 dir = computeDir(pos);"
				"vec3 view = normalize(pos - n_Cam);"
				"float dist = length(dir);"
				"dir /= dist;"
				"float NoL = dot(normal, dir);"
				"float att = attenuate(dist);"
				"n_Out = vec4(n_LightColor * NoL * att, 1.0);"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader[type];
}

ShaderCombinaison *getCompositionShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"uniform sampler2D n_D;"
			"uniform samplerCube n_Cube;"
			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"uniform float n_Ambient;"

			"in vec2 n_TexCoord;"
			"in vec4 n_Position;"

			"out vec4 n_Out;"

			"vec3 unproj(vec2 C) {"
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			"vec3 unproj() {"
				"return unproj((n_Position.xy / n_Position.w) * 0.5 + 0.5);"
			"}"

			"void main() {"
				"vec4 color = texture(n_0, n_TexCoord);"
				"vec4 light = texture(n_1, n_TexCoord);"
				"n_Out = color * n_Ambient + color * light * (1.0 - n_Ambient);"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

template<LightType Type>
ShaderCombinaison *lightPass(const FrameData *data, GBufferRenderer *child) {
	ShaderCombinaison *sh = getShader(Type);
	sh->bind();

	sh->setValue("n_1", child->getFrameBuffer().getAttachement(1));
	sh->setValue("n_2", child->getFrameBuffer().getAttachement(2));
	sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
	sh->setValue("n_Inv", (data->cam->getProjectionMatrix() * data->cam->getViewMatrix()).inverse());
	sh->setValue("n_Cam", data->cam->getPosition());

	for(const Light<> *l : data->lights[Type]) {
		sh->setValue("n_LightPos", l->getPosition());
		sh->setValue("n_LightRadius", l->getRadius());
		sh->setValue("n_LightColor", l->getColor().sub(3));
		GLContext::getContext()->getScreen().draw(VertexAttribs());
	}

	return sh;
}

ShaderCombinaison *compositionPass(const FrameData *data, GBufferRenderer *child, const FrameBuffer &light) {
	ShaderCombinaison *sh = getCompositionShader();
	sh->bind();

	sh->setValue("n_0", child->getFrameBuffer().getAttachement(0));
	sh->setValue("n_1", light.getAttachement(0));
	sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
	sh->setValue("n_Inv", (data->cam->getProjectionMatrix() * data->cam->getViewMatrix()).inverse());
	sh->setValue("n_Cam", data->cam->getPosition());
	sh->setValue("n_Cube", getCube());
	sh->setValue("n_Ambient", 0.0);

	GLContext::getContext()->getScreen().draw(VertexAttribs());

	return sh;
}

DeferredShadingRenderer::DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s) : BufferedRenderer(s.isNull() ? c->getFrameBuffer().getSize() : s), lightBuffer(getFrameBuffer().getSize()), child(c) {
	lightBuffer.setDepthEnabled(false);
	buffer.setAttachmentEnabled(0, true);
	buffer.setDepthEnabled(true);
}

void *DeferredShadingRenderer::prepare() {
	core::Array<Camera<> *> arr = child->getRenderer()->getScene()->get<Camera<>>();
	if(arr.size() != 1) {
		return 0;
	}
	return new FrameData({arr.first(),
						  {child->getRenderer()->getScene()->query<PointLight<>>(*child->getRenderer()->getCamera()),
							   child->getRenderer()->getScene()->get<DirectionalLight<>>()},
						  child->prepare()});
}

void DeferredShadingRenderer::render(void *ptr) {
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	child->render(data->child);

	{
		getLightMaterial().bind();

		lightBuffer.bind();
		gl::glClear(GL_COLOR_BUFFER_BIT);

		lightPass<Directional>(data, child);
		lightPass<Point>(data, child);
	}

	{
		getCompositionMaterial().bind();

		buffer.bind();
		child->getFrameBuffer().blit(false);
		compositionPass(data, child, lightBuffer)->unbind();
	}

	delete data;
}

}
}
