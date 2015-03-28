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

namespace internal {
	template<typename T = float>
	struct Material
	{
		static concurrent::Mutex mutex;
		static bool sorted;
		static core::Array<Material<T> *> cache;

		Material() : color(1, 1, 1, 1), roughness(0), metallic(0), depthTested(true), depthWrite(true), blend(None), cull(Back) {
			mutex.lock();
			cache.append(this);
			sorted = false;
			mutex.unlock();
		}

		~Material() {
			mutex.lock();
			cache.remove(this);
			mutex.unlock();
		}

		bool operator<(const Material<T> &m) const {
			if(diffuse != m.diffuse) {
				return diffuse < m.diffuse;
			}
			if(color != m.color) {
				return color < m.color;
			}
			if(roughness != m.roughness) {
				return roughness < m.roughness;
			}
			return false;
		}

		static void updateCache() {
			if(sorted) {
				return;
			}
			mutex.lock();
			cache.sort([](const Material<T> *a, const Material<T> *b) { return a->operator<(*b); });
			for(uint i = 0; i != cache.size(); i++) {
				cache[i]->index = i;
			}
			sorted = true;
			mutex.unlock();
		}

		Color<T> color;
		T roughness;
		T metallic;
		bool depthTested;
		bool depthWrite;

		BlendMode blend;
		CullMode cull;

		Texture diffuse;

		core::Map<core::String, Texture> textures;

		uint index;
	};


	template<typename T>
	core::Array<Material<T> *> Material<T>::cache = core::Array<Material<T> *>();

	template<typename T>
	concurrent::Mutex Material<T>::mutex = concurrent::Mutex();

	template<typename T>
	bool Material<T>::sorted = true;
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

		Material(const internal::Material<T> &i) : Material(new internal::Material<T>(i)) {
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
			const internal::Material<T> *i = getInternal();
			return i ? i->color : Color<T>();
		}

		void bind() const {
			const internal::Material<T> *i = getInternal();
			const internal::Material<T> *c = GLContext::getContext()->material.operator->();
			const ShaderCombinaison *sh = GLContext::getContext()->getShader();
			if(!i) {
				i = getNull().operator->();
			}
			if(sh) {
				sh->setValue("n_Color", i->color);
				sh->setValue("n_Roughness", i->roughness);
				sh->setValue("n_Metallic", i->metallic);

				sh->setValue("n_DiffuseMul", i->diffuse.isNull() ? 0.0 : 1.0);
				sh->setValue("n_Diffuse", i->diffuse);

				for(const auto &p : i->textures) {
					sh->setValue(p._1, p._2);
				}
			} else {
				fatal("No shader or no material");
			}
			if(!c || c->depthTested != i->depthTested) {
				if(i->depthTested) {
					gl::glEnable(GL_DEPTH_TEST);
				} else {
					gl::glDisable(GL_DEPTH_TEST);
				}
			}
			if(!c || c->depthWrite != i->depthWrite) {
				gl::glDepthMask(i->depthWrite);
			}
			if(!c || c->cull != i->cull) {
				if(i->cull == DontCull) {
					gl::glDisable(GL_CULL_FACE);
				} else {
					if(!c || c->cull == DontCull) {
						gl::glEnable(GL_CULL_FACE);
					}
					gl::GLenum glc[] = {GL_BACK, GL_FRONT};
					gl::glCullFace(glc[i->cull]);
				}
			}
			if(!c || c->blend != i->blend) {
				if(i->blend == None) {
					gl::glDisable(GL_BLEND);
				} else {
					if(!c || c->blend == None) {
						gl::glEnable(GL_BLEND);
					}
					if(i->blend == Add) {
						gl::glBlendFunc(GL_ONE, GL_ONE);
					}
				}
			}

			if(isNull()) {
				GLContext::getContext()->material = getNull();
			} else {
				GLContext::getContext()->material =  *this;
			}
		}

	private:
		friend class MaterialLoader;
		friend class GLContext;

		Material(const assets::Asset<internal::Material<T>> &t) : assets::Asset<internal::Material<T>>(t) {
		}

		Material(internal::Material<T> *i) : assets::Asset<internal::Material<T>>(std::move(i)) {
		}

		const internal::Material<T> *getInternal() const {
			return this->isValid() ? this->operator->() : (const internal::Material<T> *)0;
		}

		uint getInternalIndex() const {
			internal::Material<T>::updateCache();
			const internal::Material<T> *i = getInternal();
			return i ? i->index : 0;
		}

};

static_assert(!IsThreadSafe<Material<>>::value, "n::graphics::Material<T> should not be treated as thread-safe");

}
}

#endif // N_GRAPHICS_MATERIAL

