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

#include "RenderQueue.h"

namespace n {
namespace graphics {

uint exp(float f) {
	void *p = &f;
	return (*(uint *)p) >> 23;
}



RenderQueue::RenderQueue() : matrixBuffer(-1) {
}

void RenderQueue::insert(const core::Functor<void(RenderFlag)> &f) {
	funcs.append(f);
}

void RenderQueue::insert(const core::Functor<void()> &f) {
	funcs.append([=](RenderFlag) {
		f();
	});
}

void RenderQueue::insert(const math::Matrix4<> &t, const MeshInstance &m) {
	for(SubMeshInstance *base : m) {
		insert(RenderBatch(t, base));
	}
}

void RenderQueue::insert(const RenderBatch &b) {
	batches.append(b);
}

void RenderQueue::prepare(math::Vec3 , float) {
	batches.sort([](const RenderBatch &a, const RenderBatch &b) {
		return a.getMaterial() < b.getMaterial();
	});
}

template<typename I>
void drawRangeInstanciate(I begin, I end, RenderFlag flags) {
	if(end == begin + 1) {
		(*begin)(flags, 1, 0);
		return;
	}
	uint c = 0;
	I inst = begin++;
	for(I i = begin; i != end; i++) {
		if(!inst->canInstanciate(*i)) {
			uint count = i - inst;
			(*inst)(flags, count, c);
			c += count;
			inst = i;
		}
	}
	(*inst)(flags, end - inst, c);
}

void RenderQueue::operator()(RenderFlag flags) {
	uint matIndex = 0;
	core::Array<RenderBatch>::const_iterator drawIt = batches.begin();
	GLContext::getContext()->setMatrixBuffer(matrixBuffer);
	for(core::Array<RenderBatch>::const_iterator it = batches.begin(); it != batches.end(); it++) {
		if(matIndex == matrixBuffer.getSize()) {
			drawRangeInstanciate(drawIt, it, flags);
			drawIt = it;
			matIndex = 0;
		}
		matrixBuffer[matIndex++] = it->getMatrix();
	}
	if(matIndex) {
		drawRangeInstanciate(drawIt, batches.cend(), flags);
	}
	for(const core::Functor<void(RenderFlag)> &f : funcs) {
		f(flags);
	}
}

}
}
