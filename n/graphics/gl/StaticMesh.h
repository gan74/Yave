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

#ifndef N_GRAPHICS_GL_STATICMESH
#define N_GRAPHICS_GL_STATICMESH

#include "Transformable.h"
#include "MeshInstance.h"
#include "Renderable.h"
#include "Context.h"
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

class StaticMesh : public Movable<float>, public Renderable
{
	public:
		StaticMesh(const MeshInstance<> &i = MeshInstance<>()) : inst(i) {
			radius = inst.getRadius();
		}

		virtual void render(RenderQueue &q) override {
			q.insert(RenderBatch(this->getTransform(), inst));
		}

		const MeshInstance<> &getMeshInstance() const {
			return inst;
		}

	private:
		void draw() const {
			inst.draw();
		}

	protected:
		MeshInstance<> inst;
		using Transformable<>::radius;
};

}
}
}


#endif
#endif // N_GRAPHICS_GL_STATICMESH

