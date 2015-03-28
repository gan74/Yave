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

namespace n {
namespace graphics {

struct FrameData
{
	Camera *cam;
	core::Array<Light *> lights;
	void *child;
};

DeferredShadingRenderer::DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s) : BufferedRenderer(s.isNull() ? c->getFrameBuffer().getSize() : s), child(c) {
	buffer.setAttachmentEnabled(0, true);
	buffer.setDepthEnabled(true);
}

void *DeferredShadingRenderer::prepare() {
	core::Array<Camera *> arr = child->getRenderer()->getScene()->get<Camera>();
	if(arr.size() != 1) {
		return 0;
	}
	return new FrameData({arr.first(), child->getRenderer()->getScene()->get<Light>(), child->prepare()});
}

void DeferredShadingRenderer::render(void *ptr) {
	FrameData *data = (FrameData *)ptr;
	child->render(data->child);

	buffer.bind();
	gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderCombinaison *sh = getShader();
	sh->bind();

	sh->setValue("n_0", child->getFrameBuffer().getAttachement(0));
	sh->setValue("n_1", child->getFrameBuffer().getAttachement(1));
	sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
	sh->setValue("n_Inv", (data->cam->getProjectionMatrix() * data->cam->getViewMatrix()).inverse());
	sh->setValue("n_Cam", data->cam->getPosition());

	getMaterial().bind();

	for(const Light *l : data->lights) {
		//std::cout<<l->getPosition()<<std::endl;
		sh->setValue("n_Dir", l->getPosition().normalized());
		GLContext::getContext()->getScreen().draw(VertexAttribs());
	}

	delete data;
}

const Material<float> &DeferredShadingRenderer::getMaterial() {
	static Material<> mat;
	if(mat.isNull()) {
		internal::Material<> i;
		i.blend = Add;
		i.depthTested = false;
		mat = Material<>(i);
	}
	return mat;
}

ShaderCombinaison *DeferredShadingRenderer::getShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		Shader<FragmentShader> *frag = new Shader<FragmentShader>("#version 420\n"
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"uniform sampler2D n_D;"
			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"uniform vec3 n_Dir;"

			"in vec2 n_Tex;"
			"in vec4 n_Pos;"

			"out vec4 n_Out;"

			"vec3 unproj(vec2 C) { "
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w; "
			"}"

			"vec3 unproj() {"
				"return unproj((n_Pos.xy / n_Pos.w) * 0.5 + 0.5);"
			"}"

			"void main() {"
				"vec3 pos = unproj();"
				"vec4 packedNR = texture(n_1, n_Tex);"
				"vec4 albedo = texture(n_0, n_Tex);"
				"vec3 normal = normalize(packedNR.xyz * 2.0 - 1.0);"
				"float NoL = dot(normal, n_Dir);"
				"n_Out = vec4(albedo.rgb * NoL, albedo.a);"
			"}");

		Shader<VertexShader> *vert = new Shader<VertexShader>("#version 420\n"
			"layout(location = 0) in vec3 n_VertexPosition;"
			"layout(location = 1) in vec3 n_VertexNormal;"
			"layout(location = 2) in vec3 n_VertexTangent;"
			"layout(location = 3) in vec2 n_VertexCoord;"

			"out vec2 n_Tex;"
			"out vec4 n_Pos;"

			"void main() {"
				"n_Pos = gl_Position = vec4(n_VertexPosition, 1.0);"
				"n_Tex = n_VertexCoord;"
			"}");

		shader = new ShaderCombinaison(frag, vert);
		if(!shader->getLogs().isEmpty()) {
			std::cerr<<shader->getLogs()<<std::endl;
		}
	}
	return shader;
}

}
}
