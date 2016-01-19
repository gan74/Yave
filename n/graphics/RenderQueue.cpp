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
#include "DrawCommandBuffer.h"

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

void RenderQueue::insert(const math::Matrix4<> &t, const MeshInstance &m, RenderFlag flags) {
	for(SubMeshInstance *base : m) {
		insert(RenderBatch(t, base, flags));
	}
}

void RenderQueue::insert(const RenderBatch &b) {
	batches.append(b);
}

void RenderQueue::prepare() {
	#warning no depth sorting
	batches.sort([](const RenderBatch &a, const RenderBatch &b) {
		return a.getMaterial().getData() < b.getMaterial().getData();
	});
}

void RenderQueue::present(RenderFlag flags) {
	N_LOG_PERF;
	DrawCommandBuffer buff;
	for(const RenderBatch &r : batches) {
		while(!buff.push(r)) {
			buff.present(flags);
		}
	}
	buff.present(flags);
	for(const core::Functor<void(RenderFlag)> &f : funcs) {
		f(flags);
	}
}

}
}
