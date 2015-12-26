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

#include "DrawCommandBuffer.h"
#include "VertexArrayFactory.h"


#include <iostream>

namespace n {
namespace graphics {

DrawCommandBuffer::DrawCommandBuffer() : matrixBuffer(2048), materialBuffer(matrixBuffer.getSize()), cmdBuffer(matrixBuffer.getSize()), index(0) {
}

DrawCommandBuffer::~DrawCommandBuffer() {
}

bool DrawCommandBuffer::push(const RenderBatch &r) {
	r.getMaterial().prepare();
	if(!index) {
		first = r.getMaterial().getData();
	} else {
		if(!r.getMaterial().getData().canInstanciate(first)) {
			return false;
		}
		if(index >= matrixBuffer.getSize()) {
			return false;
		}
	}
	matrixBuffer[index] = r.getMatrix();
	materialBuffer[index] = r.getMaterial().getBufferData();
	cmdBuffer[index] = r.getVertexArrayObject().getCmd(index);
	index++;
	return true;
}

bool DrawCommandBuffer::isFull() const {
	return index == getSize();
}

uint DrawCommandBuffer::getSize() const {
	return matrixBuffer.getSize();

}

void DrawCommandBuffer::present(RenderFlag flags) {
	if(!index) {
		return;
	}
	GLContext::getContext()->setMatrixBuffer(matrixBuffer);
	Material(first).bind(RenderFlag::NoUniforms | flags);
	ShaderInstance::getCurrent()->setBuffer("n_MaterialBuffer", materialBuffer);
	ShaderInstance::validateState();
	GLContext::getContext()->getVertexArrayFactory().bind();
	cmdBuffer.update(true);
	gl::multiDrawElementsIndirect(gl::Triangles, index);
	index = 0;
}

}
}
