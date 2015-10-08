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

core::Array<const MaterialData *> matCache;
concurrent::Mutex cacheMutex;

MaterialData *const nullData = new MaterialData();
MaterialData current;
typename MaterialData::BlendMode blendMode = MaterialData::None;

void MaterialData::addToCache() const {
	if(!index) {
		cacheMutex.lock();
		matCache.append(this);
		matCache.sort([](const MaterialData *a, const MaterialData *b) {
			if(a->renderPriority != b->renderPriority) {
				return a->renderPriority < b->renderPriority;
			}
			if(a->depthTested != b->depthTested) {
				return a->depthTested > b->depthTested;
			}
			if(a->depthWrite != b->depthWrite) {
				return a->depthWrite > b->depthWrite;
			}
			if(a->prog != b->prog) {
				return a->prog < b->prog;
			}
			if(a->cull != b->cull) {
				return a->cull < b->cull;
			}
			if(a->blend != b->blend) {
				return a->blend < b->blend;
			}
			if(a->depth != b->depth) {
				return a->depth < b->depth;
			}
			return a < b;
		});
		uint max = matCache.size();
		for(uint i = 0; i != max; i++) {
			matCache[i]->index = i + 1;
		}
		cacheMutex.unlock();
	}
}

void MaterialData::removeFromCache() {
	if(index) {
		cacheMutex.lock();
		matCache.remove(this);
		cacheMutex.unlock();
	}
}

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


	/*if(data.textures) {
		for(const auto &p : *data.textures) {
			sh->setValue(p._1, p._2);
		}r
	}*/
}

void fullBind(const MaterialData &restrict data) {
	setShaderParams(data.prog.bind(), data);

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
		switch(data.blend) {
			case MaterialData::Add:
				gl::glBlendFunc(GL_ONE, GL_ONE);
			break;

			case MaterialData::SrcAlpha:
				gl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

			default: break;
		}
		blendMode = data.blend;
	}
	current = data;
}

void Material::bind(uint flags) const {
	MaterialData data = getData();

	if(flags & ForceMaterialRebind) {
		fullBind(data);
	} else {
		if(flags & DepthWriteOnly) {
			if(current.depthWrite != data.depthWrite) {
				gl::glDepthMask(current.depthWrite = data.depthWrite);
			}
			return;
		}

		if(!(flags & RenderFlag::NoShader)) {
			setShaderParams(data.prog.bind(), data);
		}
		#ifdef N_MATERIAL_FANCYNESS
		if(current.fancy || data.fancy) {
			current.fancy = data.fancy;
		#else
			{
		#endif
			if(current.depthTested != data.depthTested) {
				if(data.depthTested) {
					gl::glEnable(GL_DEPTH_TEST);
				} else {
					gl::glDisable(GL_DEPTH_TEST);
				}
				current.depthTested = data.depthTested;
			}
			if(current.depth != data.depth) {
				gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
				gl::glDepthFunc(funcs[data.depth]);
				if(data.depth == MaterialData::Greater) {
					gl::glEnable(GL_DEPTH_CLAMP);
				} else {
					gl::glDisable(GL_DEPTH_CLAMP);
				}
				current.depth = data.depth;
			}
			if(current.depthWrite != data.depthWrite) {
				gl::glDepthMask(current.depthWrite = data.depthWrite);
			}
			if(current.cull != data.cull) {
				if(data.cull == MaterialData::DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(current.cull == MaterialData::DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[data.cull]);
				}
				current.cull = data.cull;
			}
			if(current.blend != data.blend) {
				if(data.blend == MaterialData::None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(current.blend == MaterialData::None) {
						gl::glEnable(GL_BLEND);
					}
					if(blendMode != data.blend) {
						switch(data.blend) {
							case MaterialData::Add:
								gl::glBlendFunc(GL_ONE, GL_ONE);
							break;

							case MaterialData::SrcAlpha:
								gl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							break;

							default: break;
						}
						blendMode = data.blend;
					}
				}
				current.blend = data.blend;
			}
		}
	}
}


const MaterialData &Material::getData() const {
	const MaterialData *i = getInternal();
	return i ? *i : *nullData;
}

}
}
