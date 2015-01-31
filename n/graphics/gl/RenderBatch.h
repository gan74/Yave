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
#include "Context.h"
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

class RenderBatch
{
	public:
		RenderBatch(const math::Matrix4<> &m, const MeshInstance<> &i) : instanciable(true), inst(i), matrix(m) {
		}

		void operator()() const {
			gl::Context::getContext()->setModelMatrix(matrix);
			inst.draw();
		}

		bool isInstanciable() const {
			return instanciable;
		}

	private:
		bool instanciable;

		MeshInstance<> inst;
		math::Matrix4<> matrix;
};

}
}
}
#endif

#endif // N_GRPHICS_GL_RENDERBATCH

