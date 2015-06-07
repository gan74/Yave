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

#include "Material.h"
#include "VertexArrayObject.h"
#include "FrameBuffer.h"
#include "ShaderProgram.h"

namespace n {
namespace graphics {
namespace internal {

/*Texture bumpToNormal(Texture bump) {
	const FrameBuffer *fbo = GLContext::getContext()->getFrameBuffer();
	static ShaderCombinaison *sh = 0;
	if(!sh) {
		sh = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D bump;"
			"in vec2 n_TexCoord;"
			"out vec4 n_Out;"
			"void main() {"
				"vec3 tex = texture(bump, n_TexCoord).xyz;"
				"float height = (tex.x + tex.y + tex.z) * 0.333333;"
				"vec2 nxy = vec2(dFdx(height), dFdy(height));"
				"vec3 n = vec3(nxy * 5, height);"
				"n_Out = vec4(normalize(n) * 0.5 + 0.5, 0.5);"
			"}"), graphics::ShaderProgram::NoProjectionShader);
	}
	bump.synchronize();
	FrameBuffer fb(bump.getSize());
	fb.bind();
	sh->bind();
	sh->setValue("bump", bump);
	GLContext::getContext()->getScreen().draw(VertexAttribs());
	Texture normals = fb.getAttachement(0);
	if(fbo) {
		fbo->bind();
	} else {
		FrameBuffer::unbind();
	}
	return normals;
}*/

}
}
}
