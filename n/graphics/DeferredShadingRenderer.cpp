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

const Material<float> &getMaterial() {
	static Material<> mat;
	if(mat.isNull()) {
		internal::Material<> i;
		i.blend = Add;
		i.depthTested = false;
		mat = Material<>(i);
	}
	return mat;
}

ShaderCombinaison *getShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"uniform sampler2D n_2;"
			"uniform sampler2D n_D;"
			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"uniform vec3 n_Dir;"

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

			"float sqr(float x) { return x * x; }"

			"void main() {"
				"vec3 pos = unproj();"
				"vec4 albedo = texture(n_0, n_TexCoord);"
				"vec3 normal = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				//"normal.z = sqrt(1.0 - dot(normal, normal));"
				"float NoL = dot(normal, n_Dir);"
				"n_Out = vec4(albedo.rgb * NoL, albedo.a);"
				//"n_Out = vec4(normal * 0.5 + 0.5, 1.0);"
			"}"), ShaderProgram::NoProjectionShader);
		if(!shader->getLogs().isEmpty()) {
			std::cerr<<shader->getLogs()<<std::endl;
			fatal("Unable to compiler deferred shaders.");
		}
	}
	return shader;
}


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
	FrameData *data = reinterpret_cast<FrameData *>(ptr);
	child->render(data->child);

	buffer.bind();
	child->getFrameBuffer().blit(false);
	gl::glClear(GL_COLOR_BUFFER_BIT);

	getMaterial().bind();

	ShaderCombinaison *sh = getShader();
	sh->bind();

	sh->setValue("n_0", child->getFrameBuffer().getAttachement(0));
	sh->setValue("n_1", child->getFrameBuffer().getAttachement(1));
	sh->setValue("n_2", child->getFrameBuffer().getAttachement(2));
	sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
	sh->setValue("n_Inv", (data->cam->getProjectionMatrix() * data->cam->getViewMatrix()).inverse());
	sh->setValue("n_Cam", data->cam->getPosition());

	for(const Light *l : data->lights) {
		sh->setValue("n_Dir", l->getPosition().normalized());
		GLContext::getContext()->getScreen().draw(VertexAttribs());
	}

	sh->unbind();

	delete data;
}

}
}
