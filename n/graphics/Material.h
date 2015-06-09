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

template<typename T = float>
struct MaterialData
{
	MaterialData() : color(1, 1, 1, 1), roughness(0), metallic(0), depthTested(true), depthWrite(true), blend(None), cull(Back), depth(Lesser), normalIntencity(0.5) {
	}

	Color<T> color;
	T roughness;
	T metallic;
	bool depthTested;
	bool depthWrite;

	graphics::ShaderProgram prog;

	BlendMode blend;
	CullMode cull;
	DepthMode depth;

	Texture diffuse;
	Texture normal;
	T normalIntencity;

	core::Map<core::String, Texture> textures;
};

namespace internal {
	template<typename T = float>
	struct Material : core::NonCopyable
	{

		Material(const MaterialData<T> &d = MaterialData<T>()) : data(d), index(0) {
		}

		~Material() {
			if(index) {
				mutex.lock();
				cache.remove(this);
				mutex.unlock();
			}
		}

		#define N_MAT_COMPARE(mem) if(data.mem != m.data.mem) { return data.mem < m.data.mem; }

		bool operator<(const Material<T> &m) const {
			N_MAT_COMPARE(prog)
			N_MAT_COMPARE(diffuse)
			N_MAT_COMPARE(depthWrite)
			N_MAT_COMPARE(depthTested)
			N_MAT_COMPARE(depth)
			N_MAT_COMPARE(cull)
			return false;
		}

		#undef N_MAT_COMPARE

		const MaterialData<T> data;

		uint getIndex() const {
			updateIfNeeded();
			return index;
		}

		private:
			static concurrent::Mutex mutex;
			static core::Array<const Material<T> *> cache;

			void updateIfNeeded() const {
				if(index) {
					mutex.lock();
					cache.append(this);
					cache.sort([](const Material<T> *a, const Material<T> *b) { return a->operator<(*b); });
					uint max = cache.size() + 1;
					for(uint i = 1; i != max; i++) {
						cache[i]->index = i;
					}
					mutex.unlock();
				}
			}

			mutable uint index;
	};


	template<typename T>
	core::Array<const Material<T> *> Material<T>::cache = core::Array<const Material<T> *>();

	template<typename T>
	concurrent::Mutex Material<T>::mutex = concurrent::Mutex();
}



template<typename T = float>
class Material : private assets::Asset<internal::Material<T>>
{
	static const assets::Asset<internal::Material<T>> &getNull() {
		static assets::Asset<internal::Material<T>> null(new internal::Material<T>());
		return null;
	}

	public:
		Material() : assets::Asset<internal::Material<T>>() {
		}

		Material(const MaterialData<T> &i) : Material(new internal::Material<T>(i)) {
		}

		bool operator<(const Material<T> &m) const {
			return getInternalIndex() < m.getInternalIndex();
		}

		bool isValid() const {
			return assets::Asset<internal::Material<T>>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::Material<T>>::isNull();
		}

		Color<T> getColor() const {
			return getData().color;
		}

		void bind() const {
			#warning binding shader after binding material will fail
			const internal::Material<T> *i = getInternal();
			if(i) {
				i->data.prog.bind();
			}
			const internal::Material<T> *c = GLContext::getContext()->material.operator->();
			const ShaderCombinaison *sh = GLContext::getContext()->getShader();
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
			} else {
				fatal("No shader or no material");
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
				if(i->data.depth == Greater) {
					gl::glEnable(GL_DEPTH_CLAMP);
				} else if(i->data.depth == Lesser) {
					gl::glDisable(GL_DEPTH_CLAMP);
				}
			}
			if(!c || c->data.depthWrite != i->data.depthWrite) {
				gl::glDepthMask(i->data.depthWrite);
			}
			if(!c || c->data.cull != i->data.cull) {
				if(i->data.cull == DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(!c || c->data.cull == DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[i->data.cull]);
				}
			}
			if(!c || c->data.blend != i->data.blend) {
				if(i->data.blend == None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(!c || c->data.blend == None) {
						gl::glEnable(GL_BLEND);
					}
					static BlendMode blendMode = None;
					if(blendMode != i->data.blend) {
						if(i->data.blend == Add) {
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

		MaterialData<T> getData() const {
			const internal::Material<T> *i = getInternal();
			if(i) {
				return i->data;
			}
			return MaterialData<T>();
		}

	private:
		friend class MaterialLoader;
		friend class GLContext;

		Material(const internal::Material<T> &i) : Material(new internal::Material<T>(i)) {
		}

		Material(const assets::Asset<internal::Material<T>> &t) : assets::Asset<internal::Material<T>>(t) {
		}

		Material(internal::Material<T> *i) : assets::Asset<internal::Material<T>>(std::move(i)) {
		}

		const internal::Material<T> *getInternal() const {
			return this->isValid() ? this->operator->() : (const internal::Material<T> *)0;
		}

		uint getInternalIndex() const {
			const internal::Material<T> *i = getInternal();
			return i ? i->getIndex() : 0;
		}

};

static_assert(!IsThreadSafe<Material<>>::value, "n::graphics::Material<T> should not be treated as thread-safe");

}
}

#endif // N_GRAPHICS_MATERIAL

