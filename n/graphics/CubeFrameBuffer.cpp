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

#include "CubeFrameBuffer.h"

namespace n {
namespace graphics {

CubeMap::Cube buildCube(uint size, ImageFormat format) {
	Image img(math::Vec2ui(size), format);
	return CubeMap::Cube{img, img, img, img, img, img};
}

CubeFrameBuffer::CubeFrameBuffer(uint size, ImageFormat format) : FrameBufferBase(math::Vec2ui(size)), cube(buildCube(size, format), false) {
	const FrameBufferBase *fb = GLContext::getContext()->getFrameBuffer();
	bind();

	cube.synchronize(true);

	gl::bindTexture(TextureCube, cube.getHandle());
	gl::attachFramebufferTextureCube(cube.getHandle());

	for(uint i = 0; i != 6; i++) {
		drawBuffers[i] = gl::Attachment(gl::ColorAtt0 + i);
	}
	gl::attachFramebufferTexture(gl::DepthAtt, Texture2D,  0);

	gl::drawBuffers(6, drawBuffers);

	gl::assertFboStatus();

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}
}

}
}


