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

#ifndef N_GRAPHICS_GL_SCENE_H
#define N_GRAPHICS_GL_SCENE_H

#include "Transformable.h"
#include <n/math/Volume.h>
#include <n/core/Pair.h>
#include <n/types.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

class Scene : core::NonCopyable
{
	class TypedArray
	{
		public:
			TypedArray(Transformable<> *tr) : type(*tr) {
				array.append(tr);
			}

			const Type &getType() const {
				return type;
			}

			virtual bool append(Transformable<> *tr) = 0;

		protected:
			Type type;
			core::Array<Transformable<> *> array;
	};

	template<typename U>
	class TypedArraySpec : public TypedArray
	{
		public:
			TypedArraySpec(Transformable<> *tr) : TypedArray(tr) {
			}

			virtual bool append(Transformable<> *tr) override {
				U *u = dynamic_cast<U *>(tr);
				if(u) {
					TypedArray::array.append(u);
				}
				return u;
			}
	};

	public:
		Scene() {
		}

		~Scene() {
		}

		template<typename U>
		void insert(U *tr) {
			if((void *)((Transformable<> *)tr) != (void *)tr) {
				fatal("Virtual inherited Transformable<>");
			}
			transformables.append(tr);
			Type ty(*tr);
			for(core::Pair<const Type, core::Array<Transformable<> *>> &p : typeMap) {
				if(p._1 == ty) {
					p._2.append(tr);
					return;
				}
			}
			core::Array<Transformable<> *> a;
			a.append(tr);
			typeMap.append(core::Pair<const Type, core::Array<Transformable<> *>>(ty, a));
		}

		template<typename U = Transformable<>>
		core::Array<U *> get() {
			if(std::is_same<U, Transformable<>>::value) {
				const core::Array<U *> *arr = reinterpret_cast<const core::Array<U *> *>(&transformables);
				return *arr;
			}
			core::Array<U *> array(transformables.size());
			for(const core::Pair<const Type, core::Array<Transformable<> *>> &p : typeMap) {
				if(!p._2.isEmpty()) {
					if(dynamic_cast<const U *>(p._2.first())) {
						const core::Array<U *> *arr = (reinterpret_cast<const core::Array<U *> *>(&p._2));
						array.append(*arr);
					}
				}
			}
			return array;
		}

		template<typename U = Transformable<>, typename V>
		core::Array<U *> query(const V &vol) {
			return get<U>().filtered([&](U *u) {
				return vol.isInside(u->getTransform().getPosition(), u->getRadius());
			});
		}

	private:
		core::Array<Transformable<> *> transformables;
		core::Array<core::Pair<const Type, core::Array<Transformable<> *>>> typeMap;


};

}
}
}

#endif
#endif // N_GRAPHICS_GL_SCENE_H
