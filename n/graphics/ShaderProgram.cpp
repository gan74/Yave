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
#include "ShaderCombinaison.h"

namespace n {
namespace graphics {

typedef internal::ShaderProgramCombinaison Combinaison;

static const Shader<FragmentShader> *defaultFrag = 0;
static const Shader<VertexShader> *defaultVert = 0;
static const Shader<GeometryShader> *defaultGeom = 0;

namespace internal {

ShaderProgram::ShaderProgram(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom) : base({frag, vert, geom}) {
}

ShaderProgram::~ShaderProgram() {
	for(const core::Pair<const Combinaison, ShaderCombinaison *> &p : shaders) {
		delete p._2;
	}
}

ShaderProgramCombinaison ShaderProgram::getCombinaison() const {
	Combinaison c = base;
	c.shaders.frag = c.shaders.frag ? c.shaders.frag : (defaultFrag ? defaultFrag : graphics::ShaderProgram::getStandardFragmentShader());
	c.shaders.vert = c.shaders.vert ? c.shaders.vert : (defaultVert ? defaultVert : graphics::ShaderProgram::getStandardVertexShader());
	c.shaders.geom = c.shaders.geom ? c.shaders.geom : defaultGeom;
	return c;
}

}

Shader<FragmentShader> *ShaderProgram::getStandardFragmentShader() {
	static Shader<FragmentShader> *def = new Shader<FragmentShader>(
					"layout(location = 0) out vec4 n_0;"

					"uniform vec4 n_Color;"
					"uniform float n_Roughness;"
					"uniform float n_Metallic;"
					"uniform float n_DiffuseMul;"
					"uniform sampler2D n_DiffuseMap;"

					"in vec3 n_Position;"
					"in vec3 n_Normal;"
					"in vec2 n_TexCoord;"

					"void main() {"
						"vec4 color = n_Color * mix(vec4(1.0), texture(n_DiffuseMap, n_TexCoord), n_DiffuseMul);"
						"n_0 = n_gbuffer0(color, n_Normal, n_Roughness, n_Metallic);"
					"}");
	return def;
}

Shader<VertexShader> *ShaderProgram::getStandardVertexShader(StandardVertexShader type) {
	static Shader<VertexShader> **def = 0;
	if(!def) {
			def = new Shader<VertexShader>*[2];
			def[ProjectionShader] = new Shader<VertexShader>(
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
						"out int n_VertexID;"

						"void main() {"
							"vec4 model = n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
							"gl_Position = n_ScreenPosition = n_ViewProjectionMatrix * model;"
							//"gl_Position *= gl_Position.w;"//-----------------------------------------------------------------
							"n_VertexID = gl_VertexID;"
							"n_Position = model.xyz;"
							"n_View = normalize(n_Camera - model.xyz);"
							"n_Normal = mat3(n_ModelMatrix) * n_VertexNormal;"
							"n_Tangent = mat3(n_ModelMatrix) * n_VertexTangent;"
							"n_TexCoord = n_VertexCoord;"
							"n_Binormal = cross(n_Normal, n_Tangent);"
						"}");
			def[NoProjectionShader] = new Shader<VertexShader>(
						"layout(location = 0) in vec3 n_VertexPosition;"
						"layout(location = 1) in vec3 n_VertexNormal;"
						"layout(location = 2) in vec3 n_VertexTangent;"
						"layout(location = 3) in vec2 n_VertexCoord;"

						"out vec3 n_Position;"
						"out vec4 n_ScreenPosition;"
						"out vec3 n_Normal;"
						"out vec3 n_Tangent;"
						"out vec3 n_Binormal;"
						"out vec2 n_TexCoord;"
						"out int n_VertexID;"

						"void main() {"
							"gl_Position = n_ScreenPosition = vec4(n_Position = n_VertexPosition, 1.0);"
							"n_VertexID = gl_VertexID;"
							"n_Normal = n_VertexNormal;"
							"n_Tangent = n_VertexTangent;"
							"n_TexCoord = n_VertexCoord;"
							"n_Binormal = cross(n_Normal, n_Tangent);"
						"}");
	}
	return def[type];
}

ShaderProgram::ShaderProgram() : ptr(getNullProgram()) {
}

ShaderProgram::ShaderProgram(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom) : ptr(new internal::ShaderProgram(frag, vert, geom)) {
}

ShaderProgram::ShaderProgram(const Shader<FragmentShader> *frag, StandardVertexShader vert, const Shader<GeometryShader> *geom) : ptr(new internal::ShaderProgram(frag, getStandardVertexShader(vert), geom)) {
}

const ShaderCombinaison *ShaderProgram::bind() const {
	Combinaison c = ptr->getCombinaison();
	core::Map<Combinaison, ShaderCombinaison *>::const_iterator it = ptr->shaders.find(c);
	ShaderCombinaison *n = 0;
	if(it == ptr->shaders.end()) {
		ptr->shaders[c] = n = new ShaderCombinaison(c.shaders.frag, c.shaders.vert, c.shaders.geom);
	} else {
		n = (*it)._2;
	}
	n->bind();
	GLContext::getContext()->program = ptr;
	return n;
}

bool ShaderProgram::isActive() const {
	return ptr == GLContext::getContext()->program;
}

void ShaderProgram::setDefaultShader(const Shader<FragmentShader> *s) {
	defaultFrag = s;
	if(GLContext::getContext()->program && !GLContext::getContext()->program->base.shaders.frag) {
		rebind();
	}
}

void ShaderProgram::setDefaultShader(const Shader<VertexShader> *s) {
	defaultVert = s;
	if(GLContext::getContext()->program && !GLContext::getContext()->program->base.shaders.vert) {
		rebind();
	}
}

void ShaderProgram::setDefaultShader(const Shader<GeometryShader> *s) {
	defaultGeom = s;
	if(GLContext::getContext()->program && !GLContext::getContext()->program->base.shaders.geom) {
		rebind();
	}
}

bool ShaderProgram::isDefaultShader(const Shader<FragmentShader> *s) {
	return defaultFrag == s;
}

bool ShaderProgram::isDefaultShader(const Shader<VertexShader> *s) {
	return defaultVert == s;
}

bool ShaderProgram::isDefaultShader(const Shader<GeometryShader> *s) {
	return defaultGeom == s;
}

void ShaderProgram::rebind() {
	auto p = GLContext::getContext()->program;
	ShaderProgram(GLContext::getContext()->program).bind();
	GLContext::getContext()->program = p;
}

const ShaderProgram::ShaderProgramPtr &ShaderProgram::getNullProgram() {
	static ShaderProgramPtr null(new internal::ShaderProgram(0, 0, 0));
	return null;
}

}
}
