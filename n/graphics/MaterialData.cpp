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

#include "MaterialData.h"
#include "GL.h"

namespace n {
namespace graphics {

MaterialSurfaceData::MaterialSurfaceData() : color(1, 1, 1, 1), metallic(0), normalIntencity(0.5) {
}

MaterialBufferData MaterialSurfaceData::toBufferData() const {
	return MaterialBufferData{color,
							  metallic,
							  diffuse.getBindlessId() ? 1.f : 0.f,
							  normal.getBindlessId() ? normalIntencity : 0.f,
							  roughness.getBindlessId() ? 1.f : 0.f,
							  diffuse.getBindlessId(),
							  normal.getBindlessId(),
							  roughness.getBindlessId(),
							  {0, 0}};
}

bool MaterialSurfaceData::synchronize(bool immediate) const {
	bool r = true;
	r &= diffuse.synchronize(immediate);
	r &= normal.synchronize(immediate);
	r &= roughness.synchronize(immediate);
	return r;
}

MaterialRenderData::MaterialRenderData() : renderPriority(DefaultPriority), depthTested(true), depthWrite(true), depthSortable(true), blendMode(None), depthMode(Lesser), cullMode(Back) {
}

void MaterialRenderData::bind(uint flags) const {
	gl::setDepthMask(depthWrite);
	if(!(flags & DepthWriteOnly)) {
		gl::setEnabled(gl::DepthTest, depthTested);
		gl::setDepthMode(DepthMode(depthMode));
		gl::setEnabled(gl::DepthClamp, depthMode == DepthMode::Greater);
		gl::setCullFace(CullMode(cullMode));
		gl::setBlendMode(BlendMode(blendMode));
	}
}

uint32 MaterialRenderData::toUint() const {
	return *reinterpret_cast<const uint32 *>(this);
}


void MaterialData::bind(uint flags) const {
	if(flags & DepthWriteOnly) {
		gl::setDepthMask(render.depthWrite);
		return;
	}
	render.bind(flags);
	if(!(flags & RenderFlag::NoShader)) {
		prog.bind();
	}
}

bool MaterialData::canInstanciate(const MaterialData &m) const {
	return canInstanciate(m.render, m.prog);
}

bool MaterialData::canInstanciate(const MaterialRenderData &r, const ShaderProgram &p) const {
	return prog == p && (render.toUint() & MaterialRenderData::PriorityMask) == (r.toUint() & MaterialRenderData::PriorityMask);
}

bool MaterialData::synchronize(bool immediate) const {
	return surface.synchronize(immediate);
}

}
}
