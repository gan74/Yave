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

#ifndef N_GRAPHICS_DEFERREDCOMMON
#define N_GRAPHICS_DEFERREDCOMMON

#include "TriangleBuffer.h"
#include "VertexArrayObject.h"
#include "Light.h"
#include "Material.h"
#include "GL.h"

namespace n {
namespace graphics {

enum LightType
{
	Point,
	Directional,
	Box,

	Max
};

template<LightType Type>
const Material &getLightMaterial() {
	static Material mat[LightType::Max];
	if(mat[Type].isNull()) {
		MaterialData i;
		i.blend = MaterialData::Add;
		i.depthWrite = false;
		if(Type == Directional) {
			i.depth = MaterialData::Always;
		} else {
			i.depth = MaterialData::Greater;
			i.cull = MaterialData::Front;
		}
		mat[Type] = Material(i);
	}
	return mat[Type];
}

const VertexArrayObject<> &getSphere();
const VertexArrayObject<> &getBox();
core::String getBRDFs();

}
}

#endif // N_GRAPHICS_DEFERREDCOMMON

