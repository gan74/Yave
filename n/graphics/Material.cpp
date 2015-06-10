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
#include "ShaderProgram.h"

#include <n/defines.h>

namespace n {
namespace graphics {
namespace internal {

core::Array<const Material *> Material::cache = core::Array<const Material *>();
concurrent::Mutex Material::mutex = concurrent::Mutex();

}

static internal::Material *const nullInternal = new internal::Material();
static const internal::Material *currentMat = 0;
static typename MaterialData::BlendMode blendMode = MaterialData::None;


void setShaderParams(const ShaderCombinaison *restrict sh, const MaterialData &restrict data) {
	sh->setValue("n_Color", data.color);
	sh->setValue("n_Roughness", data.roughness);
	sh->setValue("n_Metallic", data.metallic);

	sh->setValue("n_DiffuseMap", data.diffuse);
	sh->setValue("n_DiffuseMul", data.diffuse.isNull() ? 0.0 : 1.0);

	sh->setValue("n_NormalMap", data.normal);
	sh->setValue("n_NormalMul", data.normal.isNull() ? 0.0 : data.normalIntencity);

	/*if(data.textures) {
		for(const auto &p : *data.textures) {
			sh->setValue(p._1, p._2);
		}
	}*/
}

void fullBind(const MaterialData &restrict data) {
	const ShaderCombinaison *restrict sh = data.prog.bind();
	setShaderParams(sh, data);

	if(data.depthTested) {
		gl::glEnable(GL_DEPTH_TEST);
	} else {
		gl::glDisable(GL_DEPTH_TEST);
	}

	gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
	gl::glDepthFunc(funcs[data.depth]);

	if(data.depth == MaterialData::Greater) {
		gl::glEnable(GL_DEPTH_CLAMP);
	} else {
		gl::glDisable(GL_DEPTH_CLAMP);
	}

	gl::glDepthMask(data.depthWrite);

	if(data.cull == MaterialData::DontCull) {
		gl::glDisable(GL_CULL_FACE);
	} else {
		gl::glEnable(GL_CULL_FACE);
		gl::GLenum glc[] = {GL_BACK, GL_FRONT};
		gl::glCullFace(glc[data.cull]);
	}

	if(data.blend == MaterialData::None) {
		gl::glDisable(GL_BLEND);
	} else {
		gl::glEnable(GL_BLEND);
		if(data.blend == MaterialData::Add) {
			gl::glBlendFunc(GL_ONE, GL_ONE);
		}
		blendMode = data.blend;
	}
}

void Material::bind(uint flags) const {
	const internal::Material *restrict intern = getInternal();
	MaterialData data = intern ? intern->data : MaterialData();

	const internal::Material *c = currentMat;
	if(!c || flags & ForceMaterialRebind) {
		fullBind(data);
	} else {
		MaterialData curr = c->data;

		if(flags & DepthWriteOnly) {
			if(curr.depthWrite != data.depthWrite) {
				gl::glDepthMask(data.depthWrite);
			}
			return;
		}

		const ShaderCombinaison *sh = (flags & RenderFlag::FastDepth && data.prog.isDefaultProgram()) ? 0 :
										(flags & RenderFlag::NoShader ? GLContext::getContext()->getShader() : data.prog.bind());

		if(sh) {
			setShaderParams(sh, data);
		}
		#ifdef N_MATERIAL_FANCYNESS
		if(curr.fancy || data.fancy)
		#endif
		{
			if(curr.depthTested != data.depthTested) {
				if(data.depthTested) {
					gl::glEnable(GL_DEPTH_TEST);
				} else {
					gl::glDisable(GL_DEPTH_TEST);
				}
			}
			if(curr.depth != data.depth) {
				gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
				gl::glDepthFunc(funcs[data.depth]);
				if(data.depth == MaterialData::Greater) {
					gl::glEnable(GL_DEPTH_CLAMP);
				} else {
					gl::glDisable(GL_DEPTH_CLAMP);
				}
			}
			if(curr.depthWrite != data.depthWrite) {
				gl::glDepthMask(data.depthWrite);
			}
			if(curr.cull != data.cull) {
				if(data.cull == MaterialData::DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(curr.cull == MaterialData::DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[data.cull]);
				}
			}
			if(curr.blend != data.blend) {
				if(data.blend == MaterialData::None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(curr.blend == MaterialData::None) {
						gl::glEnable(GL_BLEND);
					}
					if(blendMode != data.blend) {
						if(data.blend == MaterialData::Add) {
							gl::glBlendFunc(GL_ONE, GL_ONE);
						}
						blendMode = data.blend;
					}
				}
			}
		}
	}
	currentMat = intern ? intern : nullInternal;
}

}
}
