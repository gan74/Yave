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
uint16 blendMode = MaterialData::None;


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
		gl::glEnable(GL_DEPTH_TEST);
	} else {
		gl::glDisable(GL_DEPTH_TEST);
	}

	gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
	gl::glDepthFunc(funcs[data.fancy.depth]);

	if(data.fancy.depth == MaterialData::Greater) {
		gl::glEnable(GL_DEPTH_CLAMP);
	} else {
		gl::glDisable(GL_DEPTH_CLAMP);
	}

	gl::glDepthMask(data.fancy.depthWrite);

	if(data.fancy.cull == MaterialData::DontCull) {
		gl::glDisable(GL_CULL_FACE);
	} else {
		gl::glEnable(GL_CULL_FACE);
		gl::GLenum glc[] = {GL_BACK, GL_FRONT};
		gl::glCullFace(glc[data.fancy.cull]);
	}

	if(data.fancy.blend == MaterialData::None) {
		gl::glDisable(GL_BLEND);
	} else {
		gl::glEnable(GL_BLEND);
		switch(data.fancy.blend) {
			case MaterialData::Add:
				gl::glBlendFunc(GL_ONE, GL_ONE);
			break;

			case MaterialData::SrcAlpha:
				gl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

			default: break;
		}
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
				gl::glDepthMask(current.fancy.depthWrite = data.fancy.depthWrite);
			}
			return;
		}

		if(!(flags & RenderFlag::NoShader)) {
			setShaderParams(data.prog.bind(), data);
		}
		{
			if(current.fancy.depthTested != data.fancy.depthTested) {
				if(data.fancy.depthTested) {
					gl::glEnable(GL_DEPTH_TEST);
				} else {
					gl::glDisable(GL_DEPTH_TEST);
				}
				current.fancy.depthTested = data.fancy.depthTested;
			}
			if(current.fancy.depth != data.fancy.depth) {
				gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
				gl::glDepthFunc(funcs[data.fancy.depth]);
				if(data.fancy.depth == MaterialData::Greater) {
					gl::glEnable(GL_DEPTH_CLAMP);
				} else {
					gl::glDisable(GL_DEPTH_CLAMP);
				}
				current.fancy.depth = data.fancy.depth;
			}
			if(current.fancy.depthWrite != data.fancy.depthWrite) {
				gl::glDepthMask(current.fancy.depthWrite = data.fancy.depthWrite);
			}
			if(current.fancy.cull != data.fancy.cull) {
				if(data.fancy.cull == MaterialData::DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(current.fancy.cull == MaterialData::DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[data.fancy.cull]);
				}
				current.fancy.cull = data.fancy.cull;
			}
			if(current.fancy.blend != data.fancy.blend) {
				if(data.fancy.blend == MaterialData::None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(current.fancy.blend == MaterialData::None) {
						gl::glEnable(GL_BLEND);
					}
					if(blendMode != data.fancy.blend) {
						switch(data.fancy.blend) {
							case MaterialData::Add:
								gl::glBlendFunc(GL_ONE, GL_ONE);
							break;

							case MaterialData::SrcAlpha:
								gl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							break;

							default: break;
						}
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
