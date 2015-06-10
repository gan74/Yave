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

#ifndef N_GRAPHICS_MATERIAL
#define N_GRAPHICS_MATERIAL

#include <n/math/Vec.h>
#include "Texture.h"
#include "ShaderCombinaison.h"

namespace n {
namespace graphics {

enum RenderFlag
{
	None = 0x00,
	FastDepth = 0x01
};

struct MaterialData
{
	enum CullMode {
		DontCull = 2,
		Back = 0,
		Front = 1
	};

	enum BlendMode {
		None,
		Add
	};

	enum DepthMode {
		Lesser,
		Greater,
		Always
	};

	MaterialData() : color(1, 1, 1, 1), roughness(0), metallic(0), depthTested(true), depthWrite(true), blend(None), cull(Back), depth(Lesser), normalIntencity(0.5) {
	}

	Color<> color;
	float roughness;
	float metallic;
	bool depthTested;
	bool depthWrite;

	graphics::ShaderProgram prog;

	BlendMode blend;
	CullMode cull;
	DepthMode depth;

	Texture diffuse;
	Texture normal;
	float normalIntencity;

	core::Map<core::String, Texture> textures;
};

namespace internal {
	struct Material : core::NonCopyable
	{

		Material(const MaterialData &d = MaterialData()) : data(d), index(0) {
		}

		~Material() {
			if(index) {
				mutex.lock();
				cache.remove(this);
				mutex.unlock();
			}
		}

		#define N_MAT_COMPARE(mem) if(data.mem != m.data.mem) { return data.mem < m.data.mem; }

		bool operator<(const Material &m) const {
			N_MAT_COMPARE(prog)
			N_MAT_COMPARE(diffuse)
			N_MAT_COMPARE(depthWrite)
			N_MAT_COMPARE(depthTested)
			N_MAT_COMPARE(depth)
			N_MAT_COMPARE(cull)
			return false;
		}

		#undef N_MAT_COMPARE

		const MaterialData data;

		uint getIndex() const {
			updateIfNeeded();
			return index;
		}

		private:
			static concurrent::Mutex mutex;
			static core::Array<const Material *> cache;

			void updateIfNeeded() const {
				if(index) {
					mutex.lock();
					cache.append(this);
					cache.sort([](const Material *a, const Material *b) { return a->operator<(*b); });
					uint max = cache.size() + 1;
					for(uint i = 1; i != max; i++) {
						cache[i]->index = i;
					}
					mutex.unlock();
				}
			}

			mutable uint index;
	};
}



class Material : private assets::Asset<internal::Material>
{
	static const assets::Asset<internal::Material> &getNull() {
		static assets::Asset<internal::Material> null(new internal::Material());
		return null;
	}

	public:
		Material() : assets::Asset<internal::Material>() {
		}

		Material(const MaterialData &i) : Material(new internal::Material(i)) {
		}

		bool operator<(const Material &m) const {
			return getInternalIndex() < m.getInternalIndex();
		}

		bool isValid() const {
			return assets::Asset<internal::Material>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::Material>::isNull();
		}

		Color<> getColor() const {
			return getData().color;
		}

		void bind(uint flags = RenderFlag::None) const {
			#warning binding shader after binding material will fail
			const internal::Material *i = getInternal();
			if(i) {
				i->data.prog.bind();
			}
			const internal::Material *c = GLContext::getContext()->material.operator->();
			const ShaderCombinaison *sh = GLContext::getContext()->getShader();

			if(flags & RenderFlag::FastDepth) {
				if(i->data.prog.isDefaultProgram()) {
					sh = 0;
				}
			}

			if(!i) {
				i = getNull().operator->();
			}
			if(sh) {
				sh->setValue("n_Color", i->data.color);
				sh->setValue("n_Roughness", i->data.roughness);
				sh->setValue("n_Metallic", i->data.metallic);

				sh->setValue("n_DiffuseMap", i->data.diffuse);
				sh->setValue("n_DiffuseMul", i->data.diffuse.isNull() ? 0.0 : 1.0);

				sh->setValue("n_NormalMap", i->data.normal);
				sh->setValue("n_NormalMul", i->data.normal.isNull() ? 0.0 : i->data.normalIntencity);

				for(const auto &p : i->data.textures) {
					sh->setValue(p._1, p._2);
				}
			}
			if(!c || c->data.depthTested != i->data.depthTested) {
				if(i->data.depthTested) {
					gl::glEnable(GL_DEPTH_TEST);
				} else {
					gl::glDisable(GL_DEPTH_TEST);
				}
			}
			if(!c || c->data.depth != i->data.depth) {
				gl::GLenum funcs[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
				gl::glDepthFunc(funcs[i->data.depth]);
				if(i->data.depth == MaterialData::Greater) {
					gl::glEnable(GL_DEPTH_CLAMP);
				} else if(i->data.depth == MaterialData::Lesser || i->data.depth == MaterialData::Always) {
					gl::glDisable(GL_DEPTH_CLAMP);
				}
			}
			if(!c || c->data.depthWrite != i->data.depthWrite) {
				gl::glDepthMask(i->data.depthWrite);
			}
			if(!c || c->data.cull != i->data.cull) {
				if(i->data.cull == MaterialData::DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(!c || c->data.cull == MaterialData::DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[i->data.cull]);
				}
			}
			if(!c || c->data.blend != i->data.blend) {
				if(i->data.blend == MaterialData::None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(!c || c->data.blend == MaterialData::None) {
						gl::glEnable(GL_BLEND);
					}
					static typename MaterialData::BlendMode blendMode = MaterialData::None;
					if(blendMode != i->data.blend) {
						if(i->data.blend == MaterialData::Add) {
							gl::glBlendFunc(GL_ONE, GL_ONE);
						}
						blendMode = i->data.blend;
					}
				}

			}
			if(isNull()) {
				GLContext::getContext()->material = getNull();
			} else {
				GLContext::getContext()->material =  this->operator->();
			}
		}

		MaterialData getData() const {
			const internal::Material *i = getInternal();
			if(i) {
				return i->data;
			}
			return MaterialData();
		}

	private:
		friend class MaterialLoader;
		friend class GLContext;

		/*Material(const internal::Material &i) : Material(new internal::Material(i)) {
		}*/

		Material(const assets::Asset<internal::Material> &t) : assets::Asset<internal::Material>(t) {
		}

		Material(internal::Material *i) : assets::Asset<internal::Material>(std::move(i)) {
		}

		const internal::Material *getInternal() const {
			return this->isValid() ? this->operator->() : (const internal::Material *)0;
		}

		uint getInternalIndex() const {
			const internal::Material *i = getInternal();
			return i ? i->getIndex() : 0;
		}

};

static_assert(!IsThreadSafe<Material>::value, "n::graphics::Material should not be treated as thread-safe");

}
}

#endif // N_GRAPHICS_MATERIAL

