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
		RenderBatch(const math::Matrix4<> &m, SubMeshInstance *i, const VertexAttribs &attr = VertexAttribs(), uint renderFlags = RenderFlag::None) : instanciable(true), flags(renderFlags), inst(i), matrix(m), attribs(attr) {
		}

		void operator()(uint renderFlags = RenderFlag::None) const {
			GLContext::getContext()->setModelMatrix(matrix);
			inst->draw(attribs, renderFlags | renderFlags);
		}

		bool isInstanciable() const {
			return instanciable;
		}

		bool operator<(const RenderBatch &o) const {
			bool inf = getMaterial() < o.getMaterial();
			if(inf) {
				return false;
			}
			if(o.getMaterial() < getMaterial()) {
				return false;
			}
			return getVertexArrayObject() < o.getVertexArrayObject();
		}

		bool canInstanciate(const RenderBatch &o) const {
			return isInstanciable() && o.isInstanciable() && inst == o.inst;
		}

		const math::Matrix4<> &getMatrix() const {
			return matrix;
		}

		const Material &getMaterial() const {
			return inst->getMaterial();
		}

		const VertexArraySubObject<> &getVertexArrayObject() const {
			return inst->getVertexArrayObject();
		}

		bool isDepthSorted() const {
			return getMaterial().getData().fancy.depthSortable;
		}

		uint getRenderPriority() const {
			return getMaterial().getData().fancy.renderPriority;
		}

	private:
		bool instanciable;
		uint flags;

		SubMeshInstance *inst;
		math::Matrix4<> matrix;
		VertexAttribs attribs;
};

}
}

#endif // N_GRPHICS_GL_RENDERBATCH

