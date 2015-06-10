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

#ifndef N_GRAPHICS_RENDERQUEUE
#define N_GRAPHICS_RENDERQUEUE

#include <n/core/Array.h>
#include <n/core/Functor.h>
#include "RenderBatch.h"

namespace n {
namespace graphics {

class RenderQueue
{
	public:
		//typedef core::Array<core::Functor<void()>>::const_iterator const_iterator;

		RenderQueue() {
		}

		void insert(const core::Functor<void()> &f) {
			funcs.append(f);
		}

		void insert(const math::Matrix4<> &t, const MeshInstance &m) {
			for(SubMeshInstance *base : m) {
				insert(RenderBatch(t, base));
			}
		}

		void insert(const RenderBatch &b) {
			batches.append(b);
		}

		template<typename U>
		RenderQueue &operator<<(const U &u) {
			insert(u);
			return *this;
		}

		void prepare() {
			batches.sort();
		}

		const core::Array<core::Functor<void()>> &getFunctions() const {
			return funcs;
		}

		const core::Array<RenderBatch> &getBatches() const {
			return batches;
		}

	private:
		core::Array<RenderBatch> batches;
		core::Array<core::Functor<void()>> funcs;
};

}
}

#endif // N_GRAPHICS_RENDERQUEUE

