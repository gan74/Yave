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

#ifndef N_GRPHICS_GL_RENDERBATCH
#define N_GRPHICS_GL_RENDERBATCH

#include <n/math/Matrix.h>
#include "MeshInstance.h"
#include "GLContext.h"

namespace n {
namespace graphics {

class RenderBatch
{
	public:
		RenderBatch(const math::Matrix4<> &m, const VertexArrayObject<> &vao, const Material &mat, RenderFlag renderFlags = RenderFlag::None) : flags(renderFlags), obj(vao), material(mat), matrix(m) {
		}

		RenderBatch(const math::Matrix4<> &m, const SubMeshInstance &sub, RenderFlag renderFlags = RenderFlag::None) : RenderBatch(m, sub.getVertexArrayObject(), sub.getMaterial(), renderFlags) {
		}

		const math::Matrix4<> &getMatrix() const {
			return matrix;
		}

		const Material &getMaterial() const {
			return material;
		}

		const VertexArrayObject<> &getVertexArrayObject() const {
			return obj;
		}

		bool isDepthSorted() const {
			return material.getData().render.depthSortable;
		}

		uint getRenderPriority() const {
			return material.getData().render.renderPriority;
		}

	private:
		RenderFlag flags;

		VertexArrayObject<> obj;
		Material material;

		math::Matrix4<> matrix;
};

}
}

#endif // N_GRPHICS_GL_RENDERBATCH

