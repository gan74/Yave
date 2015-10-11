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

#include "Material.h"
#include "VertexArrayObject.h"
#include "FrameBuffer.h"
#include "ShaderInstance.h"

#include <n/defines.h>

namespace n {
namespace graphics {

MaterialData *const nullData = new MaterialData();
MaterialData current;
uint16 blendMode = BlendMode::DontBlend;


N_FORCE_INLINE void setShaderParams(const ShaderInstance *sh restrict, const MaterialData &restrict data) {
	if(!sh) {
		return;
	}

	sh->setValue(ShaderValue::SVColor, data.color);
	sh->setValue(ShaderValue::SVMetallic, data.metallic);

	sh->setValue(ShaderValue::SVDiffuseMap, data.diffuse);
	sh->setValue(ShaderValue::SVDiffuseMul, data.diffuse.isNull() ? 0.0 : 1.0);

	sh->setValue(ShaderValue::SVNormalMap, data.normal);
	sh->setValue(ShaderValue::SVNormalMul, data.normal.isNull() ? 0.0 : data.normalIntencity);

	sh->setValue(ShaderValue::SVRoughnessMap, data.roughness);
	sh->setValue(ShaderValue::SVRoughnessMul, data.roughness.isNull() ? 0.0 : 1.0);
}

void fullBind(const MaterialData &restrict data) {
	setShaderParams(data.prog.bind(), data);

	if(data.fancy.depthTested) {
		gl::enable(gl::DepthTest);
	} else {
		gl::disable(gl::DepthTest);
	}

	gl::depthFunc(DepthMode(data.fancy.depth));

	if(data.fancy.depth == DepthMode::Greater) {
		gl::enable(gl::DepthClamp);
	} else {
		gl::disable(gl::DepthClamp);
	}

	gl::depthMask(data.fancy.depthWrite);

	if(data.fancy.cull == CullMode::DontCull) {
		gl::disable(gl::CullFace);
	} else {
		gl::enable(gl::CullFace);
		gl::cullFace(CullMode(data.fancy.cull));
	}

	if(data.fancy.blend == BlendMode::DontBlend) {
		gl::disable(gl::Blend);
	} else {
		gl::enable(gl::Blend);
		gl::blendFunc(BlendMode(data.fancy.blend));
		blendMode = data.fancy.blend;
	}
	current = data;
}

void Material::bind(uint flags) const {
	MaterialData data = getData();

	if(flags & ForceMaterialRebind) {
		fullBind(data);
	} else {
		if(flags & DepthWriteOnly) {
			if(current.fancy.depthWrite != data.fancy.depthWrite) {
				gl::depthMask((current.fancy.depthWrite = data.fancy.depthWrite));
			}
			return;
		}

		if(!(flags & RenderFlag::NoShader)) {
			setShaderParams(data.prog.bind(), data);
		}
		{
			if(current.fancy.depthTested != data.fancy.depthTested) {
				if(data.fancy.depthTested) {
					gl::enable(gl::DepthTest);
				} else {
					gl::disable(gl::DepthTest);
				}
				current.fancy.depthTested = data.fancy.depthTested;
			}
			if(current.fancy.depth != data.fancy.depth) {
				gl::depthFunc(DepthMode(data.fancy.depth));
				if(data.fancy.depth == DepthMode::Greater) {
					gl::enable(gl::DepthClamp);
				} else {
					gl::disable(gl::DepthClamp);
				}
				current.fancy.depth = data.fancy.depth;
			}
			if(current.fancy.depthWrite != data.fancy.depthWrite) {
				gl::depthMask((current.fancy.depthWrite = data.fancy.depthWrite));
			}
			if(current.fancy.cull != data.fancy.cull) {
				if(data.fancy.cull == CullMode::DontCull) {
					gl::disable(gl::CullFace);
				} else {
					if(current.fancy.cull == CullMode::DontCull) {
						gl::enable(gl::CullFace);
					}
					gl::cullFace(CullMode(data.fancy.cull));
				}
				current.fancy.cull = data.fancy.cull;
			}
			if(current.fancy.blend != data.fancy.blend) {
				if(data.fancy.blend == BlendMode::DontBlend) {
					gl::disable(gl::Blend);
				} else {
					if(current.fancy.blend == BlendMode::DontBlend) {
						gl::enable(gl::Blend);
					}
					if(blendMode != data.fancy.blend) {
						gl::blendFunc(BlendMode(data.fancy.blend));
						blendMode = data.fancy.blend;
					}
				}
				current.fancy.blend = data.fancy.blend;
			}
		}
	}
}


const MaterialData &Material::getData() const {
	const MaterialData *i = operator->();
	return i ? *i : *nullData;
}

}
}
