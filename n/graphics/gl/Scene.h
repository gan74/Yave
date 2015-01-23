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
#include <n/core/Pair.h>
#include <n/types.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class Scene : core::NonCopyable
{
	class TypedArray
	{
		public:
			TypedArray(Transformable<T> *tr) : type(*tr) {
				array.append(tr);
			}

			const Type &getType() const {
				return type;
			}

			virtual bool append(Transformable<T> *tr) = 0;

		protected:
			Type type;
			core::Array<Transformable<T> *> array;
	};

	template<typename U>
	class TypedArraySpec : public TypedArray
	{
		public:
			TypedArraySpec(Transformable<T> *tr) : TypedArray(tr) {
			}

			virtual bool append(Transformable<T> *tr) override {
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
		void add(U *tr) {
			if((void *)((Transformable<T> *)tr) != (void *)tr) {
				fatal("Virtual inherited Transformable<>");
			}
			transformables.append(tr);
			Type ty(*tr);
			for(core::Pair<const Type, core::Array<Transformable<T> *>> &p : typeMap) {
				if(p._1 == ty) {
					p._2.append(tr);
					return;
				}
			}
			core::Array<Transformable<T> *> a;
			a.append(tr);
			typeMap.append(core::Pair<const Type, core::Array<Transformable<T> *>>(ty, a));
		}

		template<typename U>
		core::Array<U *> get() {
			if(std::is_same<U, Transformable<T>>::value) {
				return *reinterpret_cast<const core::Array<U *> *>(&transformables);
			}
			core::Array<U *> array(transformables.size());
			for(const core::Pair<const Type, core::Array<Transformable<T> *>> &p : typeMap) {
				if(!p._2.isEmpty()) {
					if(dynamic_cast<const U *>(p._2.first())) {
						array.append(*(reinterpret_cast<const core::Array<U *> *>(&p._2)));
					}
				}
			}
			return array;
		}

	private:
		core::Array<Transformable<T> *> transformables;
		core::Array<core::Pair<const Type, core::Array<Transformable<T> *>>> typeMap;


};

}
}
}

#endif
#endif // N_GRAPHICS_GL_SCENE_H
