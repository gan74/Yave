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

#include "ShaderProgram.h"
#include "GBufferRenderer.h"
#include "ShaderInstanceFactory.h"

namespace n {
namespace graphics {

core::SmartPtr<ShaderProgram::Data> ShaderProgram::nullData = new ShaderProgram::Data{{0}, {ShaderProgram::Data::Replace, ShaderProgram::Data::Replace, ShaderProgram::Data::Replace}};

Shader<VertexShader> *getStandardVertexShader(ShaderProgram::StandardVertexShader type = ShaderProgram::ProjectionShader) {
	static Shader<VertexShader> **def = 0;
	if(!def) {

			def = new Shader<VertexShader>*[2];
			def[ShaderProgram::ProjectionShader] = new Shader<VertexShader>(
						"layout(location = 0) in vec3 n_VertexPosition;"
						"layout(location = 1) in vec3 n_VertexNormal;"
						"layout(location = 2) in vec3 n_VertexTangent;"
						"layout(location = 3) in vec2 n_VertexCoord;"
						"uniform mat4 n_ViewProjectionMatrix;"
						"uniform mat4 n_ModelMatrix;"
						"uniform vec3 n_Camera;"

						"out vec3 n_Position;"
						"out vec4 n_ScreenPosition;"
						"out vec3 n_Normal;"
						"out vec3 n_Tangent;"
						"out vec3 n_Binormal;"
						"out vec3 n_View;"
						"out vec2 n_TexCoord;"

						"void main() {"
							"vec4 model = n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
							"gl_Position = n_ScreenPosition = n_ViewProjectionMatrix * model;"
							//"gl_Position *= gl_Position.w;"//-----------------------------------------------------------------
							"n_Position = model.xyz;"
							"n_View = normalize(n_Camera - model.xyz);"
							"n_Normal = mat3(n_ModelMatrix) * n_VertexNormal;"
							"n_Tangent = mat3(n_ModelMatrix) * n_VertexTangent;"
							"n_TexCoord = n_VertexCoord;"
							"n_Binormal = cross(n_Normal, n_Tangent);"
						"}");
			def[ShaderProgram::NoProjectionShader] = new Shader<VertexShader>(
						"layout(location = 0) in vec3 n_VertexPosition;"
						"layout(location = 1) in vec3 n_VertexNormal;"
						"layout(location = 2) in vec3 n_VertexTangent;"
						"layout(location = 3) in vec2 n_VertexCoord;"

						"out vec3 n_Position;"
						"out vec4 n_ScreenPosition;"
						"out vec3 n_Normal;"
						"out vec3 n_Tangent;"
						"out vec3 n_Binormal;"
						"out vec3 n_View;"
						"out vec2 n_TexCoord;"

						"void main() {"
							"gl_Position = n_ScreenPosition = vec4(n_Position = n_VertexPosition, 1.0);"
							"n_Normal = n_VertexNormal;"
							"n_Tangent = n_VertexTangent;"
							"n_TexCoord = n_VertexCoord;"
							"n_Binormal = cross(n_Normal, n_Tangent);"
						"}");
	}
	return def[type];
}

Shader<FragmentShader> *getDefaultFrag() {
	//return GBufferRenderer::getShader();
	static Shader<FragmentShader> *def = new Shader<FragmentShader>(
					"layout(location = 0) out vec4 n_0;"

					"uniform vec4 n_Color;"
					"uniform float n_Roughness;"
					"uniform float n_Metallic;"
					"uniform float n_DiffuseMul;"
					"uniform sampler2D n_DiffuseMap;"

					"in vec2 n_TexCoord;"

					"void main() {"
						"vec4 color = n_Color * mix(vec4(1.0), texture(n_DiffuseMap, n_TexCoord), n_DiffuseMul);"
						"n_0 = n_gbuffer0(color, n_Normal, n_Roughness, n_Metallic);"
					"}");
	return def;
}


ShaderProgram::ShaderProgram(Shader<FragmentShader> *frag, Shader<VertexShader> *vert, Shader<GeometryShader> *geom) : data(new Data{{frag, vert, geom}, {Data::Replace, Data::Replace, Data::Replace}}) {
}

ShaderProgram::ShaderProgram(Shader<FragmentShader> *frag, StandardVertexShader std, Shader<GeometryShader> *geom) : ShaderProgram(frag, getStandardVertexShader(std), geom) {
}

ShaderProgram::ShaderProgram() : data(nullData) {
}

ShaderProgram::ShaderProgram(core::SmartPtr<Data> ptr) : data(ptr) {
}

ShaderProgram::~ShaderProgram() {
}


bool ShaderProgram::isCurrent() const {
	return GLContext::getContext()->shader == data;
}

void ShaderProgram::bind() const {
	if(GLContext::getContext()->shader != data) {
		rebind();
	}
}



void ShaderProgram::rebind() const {
	Shader<FragmentShader> *frag = (Shader<FragmentShader> *)(data->bases[0] ? data->bases[0] : ShaderBase::currents[0]);
	Shader<VertexShader> *vert = (Shader<VertexShader> *)(data->bases[1] ? data->bases[1] : ShaderBase::currents[1]);
	Shader<GeometryShader> *geom = (Shader<GeometryShader> *)(data->bases[2] ? data->bases[2] : ShaderBase::currents[2]);
	ShaderInstance *inst = GLContext::getContext()->getShaderFactory().get(frag, vert, geom);
	ShaderInstance::current = inst;
	if(inst) {
		inst->bind();
		for(UniformBase *u : uniforms) {
			delete u;
		}
	}  else {
		for(UniformBase *u : uniforms) {
			u->set(inst);
			delete u;
		}
	}
	uniforms.clear();
}

void ShaderProgram::unbind() const {
	if(GLContext::getContext()->shader != data) {
		ShaderProgram(nullData).bind();
	}
}



bool ShaderProgram::operator==(const ShaderProgram &p) const {
	for(uint i = 0; i != 3; i++) {
		if(data->bases[i] != p.data->bases[i]) {
			return false;
		}
	}
	return true;
}

bool ShaderProgram::operator!=(const ShaderProgram &p) const {
	for(uint i = 0; i != 3; i++) {
		if(data->bases[i] != p.data->bases[i]) {
			return true;
		}
	}
	return false;
}

bool ShaderProgram::operator<(const ShaderProgram &p) const {
	return data < p.data;
}


/*void ShaderProgram::bindStandards() {
	setValue("n_ProjectionMatrix", GLContext::getContext()->getProjectionMatrix());
	setValue("n_ViewMatrix", GLContext::getContext()->getViewMatrix());
	setValue("n_ViewportSize", math::Vec2(GLContext::getContext()->getViewport()));
	setValue("n_ModelMatrix", GLContext::getContext()->getModelMatrix());
	setValue("n_ViewProjectionMatrix", GLContext::getContext()->getProjectionMatrix() * GLContext::getContext()->getViewMatrix());
}



ShaderProgram::UniformInfo ShaderProgram::getInfo(const core::String &name) const {
	return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0});
}

ShaderProgram::UniformAddr ShaderProgram::getAddr(const core::String &name) const {
	return getInfo(name).addr;
}*/


}
}

