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
#include <iostream>


namespace n {
namespace graphics {

uint exp(float f) {
	void *p = &f;
	return (*(uint *)p) >> 23;
}



RenderQueue::RenderQueue() {
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
	(b.isDepthSorted() ? sortable : notSortable).append(b);
}

void RenderQueue::prepare(math::Vec3 cpos, float max) {
	uint size = exp(max + 1) - exp(1);
	core::Array<RenderBatch> *buckets = new core::Array<RenderBatch>[size];

	for(const RenderBatch &b : sortable) {
		math::Vec3 v = (b.getMatrix() * math::Vec4(0, 0, 0, 1)).sub(3);
		float dist = std::max((cpos - v).length() - b.getVertexArrayObject().getRadius(), 0.0f);
		uint index = exp(dist + 1) - exp(1);
		if(index < size) {
			buckets[index].append(b);
		}
	}
	sortable.makeEmpty();
	for(uint i = 0; i != size; i++) {
		buckets[i].sort();
		sortable.append(buckets[i]);
	}
	delete[] buckets;
	notSortable.sort();
}

void RenderQueue::operator()(RenderFlag flags) {
	for(const RenderBatch &b : sortable) {
		b(flags);
	}
	for(const RenderBatch &b : notSortable) {
		b(flags);
	}
	for(const core::Functor<void(RenderFlag)> &f : funcs) {
		f(flags);
	}
}

}
}
