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


namespace internal {
	template<typename T = float>
	struct Material
	{
		static concurent::Mutex mutex;
		static core::Array<Material<T> *> cache;

		Material() : roughness(0), metallic(0) {
			mutex.lock();
			cache.append(this);
			mutex.unlock();
		}

		~Material() {
			mutex.lock();
			cache.remove(this);
			mutex.unlock();
		}

		bool operator<(const Material<T> &m) const {
			if(color != m.color) {
				return color < m.color;
			}
			if(roughness != m.roughness) {
				return roughness < m.roughness;
			}
			return false;
		}

		static void updateCache() {
			mutex.lock();
			cache.sort([](const Material<T> *a, const Material<T> *b) { return a->operator<(*b); });
			for(uint i = 0; i != cache.size(); i++) {
				cache[i]->index = i;
			}
			mutex.unlock();
		}

		Color<T> color;
		T roughness;
		T metallic;
		Texture diffuse;

		uint index;
	};


	template<typename T>
	core::Array<Material<T> *> Material<T>::cache = core::Array<Material<T> *>();

	template<typename T>
	concurent::Mutex Material<T>::mutex = concurent::Mutex();
}

template<typename T = float>
class Material : private assets::Asset<internal::Material<T>>
{
	public:
		Material() : assets::Asset<internal::Material<T>>() {
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
			if(i) {
				const ShaderCombinaison *sh = GLContext::getContext()->getShader();
				sh->setValue("n_Color", i->color);
				sh->setValue("n_Roughness", i->roughness);
				sh->setValue("n_Metallic", i->metallic);
			} else {
				fatal("No shader");
				#warning not implemented
			}
		}

	private:
		friend class MaterialLoader;

		Material(const assets::Asset<internal::Material<T>> &t) : assets::Asset<internal::Material<T>>(t) {
		}

		Material(internal::Material<T> *i) : assets::Asset<internal::Material<T>>(std::move(i)) {
		}

		const internal::Material<T> *getInternal() const {
			return this->isValid() ? this->operator->() : (const internal::Material<T> *)0;
		}

		uint getInternalIndex() const {
			const internal::Material<T> *i = getInternal();
			return i ? i->index : 0;
		}

};

static_assert(!IsThreadSafe<Material<>>::value, "n::graphics::Material<T> should not be treated as thread-safe");

}
}

#endif // N_GRAPHICS_MATERIAL

