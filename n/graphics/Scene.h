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

#ifndef N_GRAPHICS_SCENE_H
#define N_GRAPHICS_SCENE_H

#include "Transformable.h"
#include <n/math/Volume.h>
#include <n/core/Pair.h>
#include <n/types.h>

namespace n {
namespace graphics {

class Scene : core::NonCopyable
{

	typedef core::Pair<const Type, core::Array<Transformable<> *>> TypedArray;
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
			for(TypedArray &p : typeMap) {
				if(p._1 == ty) {
					p._2.append(tr);
					return;
				}
			}
			core::Array<Transformable<> *> a;
			a.append(tr);
			typeMap.append(TypedArray(ty, a));
		}

		template<typename U = Transformable<>>
		core::Array<U *> get() const {
			if(std::is_same<U, Transformable<>>::value) {
				const core::Array<U *> *arr = reinterpret_cast<const core::Array<U *> *>(&transformables);
				return *arr;
			}
			core::Array<U *> array(typename core::Array<U *>::Size(transformables.size()));
			for(const TypedArray &p : typeMap) {
				if(!p._2.isEmpty()) {
					U *u = dynamic_cast<U *>(p._2.first());
					if(u) {
						uint off = (uint)u - (uint)p._2.first();
						if(off) {
							array.append(p._2.mapped([=](Transformable<> *v) { return (U *)((byte *)v + off); }));
						} else {
							const core::Array<U *> *arr = (reinterpret_cast<const core::Array<U *> *>(&p._2));
							array.append(*arr);
						}
					}
				}
			}
			return array;
		}

		template<typename U = Transformable<>, typename V>
		core::Array<U *> query(const V &vol) const {
			if(std::is_same<U, Transformable<>>::value) {
				const core::Array<U *> *arr = reinterpret_cast<const core::Array<U *> *>(&transformables);
				return *arr;
			}
			core::Array<U *> array(typename core::Array<U *>::Size(transformables.size()));
			for(const TypedArray &p : typeMap) {
				if(!p._2.isEmpty()) {
					U *u = dynamic_cast<U *>(p._2.first());
					if(u) {
						uint off = (uint)u - (uint)p._2.first();
						array.append(p._2
							.filtered([&](Transformable<> *v) { return vol.isInside(v->getPosition(), v->getRadius()); })
							.mapped([=](Transformable<> *v) { return (U *)((byte *)v + off); }));

					}
				}
			}
			return array;
		}

	private:
		core::Array<Transformable<> *> transformables;
		core::Array<TypedArray> typeMap;


};

}
}

#endif // N_GRAPHICS_SCENE_H
