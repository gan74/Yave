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
	if(isFull()) {
		return false;
	}
	r.getMaterial().synchronize();
	if(isEmpty()) {
		renderData = r.getMaterial()->render;
		shader = r.getMaterial()->prog;
	} else if(!r.getMaterial()->canInstanciate(renderData, shader)) {
		return false;
	}
	matrixBuffer[index] = r.getMatrix();
	materialBuffer[index] = r.getMaterial()->surface.toBufferData();
	cmdBuffer[index] = r.getVertexArrayObject().getCmd(index);
	index++;
	return true;
}

bool DrawCommandBuffer::isFull() const {
	return index >= getCapacity();
}

bool DrawCommandBuffer::isEmpty() const {
	return !index;
}

uint DrawCommandBuffer::getCapacity() const {
	return matrixBuffer.getSize();
}

uint DrawCommandBuffer::getSize() const {
	return index;
}

void DrawCommandBuffer::present(RenderFlag flags) {
	if(isEmpty()) {
		return;
	}
	renderData.bind(flags);

	const ShaderInstance *sh = shader.bind();
	sh->setBuffer("n_MaterialBuffer", materialBuffer);
	GLContext::getContext()->setMatrixBuffer(matrixBuffer);

	ShaderInstance::validateState();
	GLContext::getContext()->getVertexArrayFactory().bind();

	cmdBuffer.update(true);
	gl::multiDrawElementsIndirect(gl::Triangles, index);
	index = 0;
}

}
}
