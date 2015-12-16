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

#include "CubeMap.h"
#include "TextureBinding.h"
#include "FrameBuffer.h"
#include "ShaderInstance.h"
#include "Material.h"
#include "VertexArrayObject.h"

namespace n {
namespace graphics {

TextureArray *buildCube(const CubeMap::Cube &cube) {
	const FrameBuffer *buff = GLContext::getContext()->getFrameBuffer();

	auto f = cube.top.getFormat();
	FrameBuffer fbo(cube.top.getSize(), false, f, f, f, f, f, f);
	ShaderInstance shader(new Shader<FragmentShader>(
		"uniform sampler2D top;"
		"uniform sampler2D bottom;"
		"uniform sampler2D right;"
		"uniform sampler2D left;"
		"uniform sampler2D front;"
		"uniform sampler2D back;"
		"layout(location = 0) out vec4 n_0;"
		"layout(location = 1) out vec4 n_1;"
		"layout(location = 2) out vec4 n_2;"
		"layout(location = 3) out vec4 n_3;"
		"layout(location = 4) out vec4 n_4;"
		"layout(location = 5) out vec4 n_5;"

		"in vec2 n_TexCoord;"

		"void main() { "
			"n_4 = texture(top, n_TexCoord);"
			"n_5 = texture(bottom, n_TexCoord);"
			"n_2 = texture(right, n_TexCoord);"
			"n_3 = texture(left, n_TexCoord);"
			"n_1 = texture(back, n_TexCoord);"
			"n_0 = texture(front, n_TexCoord);"
		"}"
	), ShaderProgram::NoProjectionShader);
	shader.setValue("top", cube.top);
	shader.setValue("bottom", cube.bottom);
	shader.setValue("right", cube.right);
	shader.setValue("left", cube.left);
	shader.setValue("front", cube.front);
	shader.setValue("back", cube.back);

	shader.bind();
	fbo.bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	if(buff) {
		buff->bind();
	}

	return new TextureArray(fbo.asTextureArray());
}

CubeMap::CubeMap(const TextureArray &array) : texArray(new TextureArray(array)), cube(0) {
}

CubeMap::CubeMap(const Cube &sides) : texArray(0), cube(new Cube(sides)) {
}

void CubeMap::upload() const {
	if(!getHandle()) {
		if(cube) {
			texArray = buildCube(*cube);
		}
		if(texArray->getSize().z() < 6) {
			logMsg("Cube map build from array with less than 6 elements", ErrorLog);
			return;
		}
		data->handle = gl::createTextureCubeView(texArray->getHandle(), 1, gl::getTextureFormat(texArray->getFormat()));
		data->parent = texArray->getHandle();

		if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
			data->bindless = gl::getTextureSamplerHandle(data->handle, GLContext::getContext()->getDefaultSampler(), hasMipmaps());
			gl::makeTextureHandleResident(data->bindless);
		}
		texArray = 0;
		cube = 0;
		internal::TextureBinding::dirty();
	}
}

bool CubeMap::synchronize(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(!cube) {
		if(!texArray->synchronize(sync)) {
			return false;
		}
	} else {
		if(!syncCube(sync)) {
			return false;
		}
	}
	if(sync) {
		data->lock.trylock();
		upload();
	} else {
		if(data->lock.trylock()) {
			CubeMap self(*this);
			GLContext::getContext()->addGLTask([=]() {
				self.upload();
			});
		}
	}
	return getHandle();
}

bool CubeMap::syncCube(bool sync) const {
	bool r = true;
	r &= cube->top.synchronize(sync);
	r &= cube->bottom.synchronize(sync);
	r &= cube->right.synchronize(sync);
	r &= cube->left.synchronize(sync);
	r &= cube->front.synchronize(sync);
	r &= cube->back.synchronize(sync);
	return r;
}

}
}
