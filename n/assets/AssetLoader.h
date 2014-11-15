/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#ifndef N_ASSETS_ASSETLOADER_H
#define N_ASSETS_ASSETLOADER_H

#include "Asset.h"
#include <n/core/Functor.h>
#include <n/core/Array.h>

namespace n {
namespace assets {

template<typename T>
class AssetLoader
{
	using VariadicType = core::Array<const std::type_info *>;

	template<typename... Ar>
	class TypeArray
	{
		template<typename W, typename... A>
		struct TypeHelper
		{
			static VariadicType types() {
				return VariadicType() + &typeid(W) + TypeHelper<A...>::types();
			}
		};

		template<typename W, typename Z>
		struct TypeHelper<W, Z>
		{
			static VariadicType types() {
				return VariadicType() + &typeid(W) + &typeid(Z);
			}
		};

		public:
			static VariadicType types() {
				VariadicType arr = TypeHelper<Ar..., NullType, NullType>::types();
				arr.pop();
				arr.pop();
				return arr;
			}
	};

	class LoaderBase
	{
		public:
			virtual VariadicType types() const = 0;
	};

	template<typename... Args>
	class LoaderFunc : public LoaderBase
	{
		public:
			template<typename U>
			LoaderFunc(U u) : func(u) {
			}

			VariadicType types() const override {
				return TypeArray<Args...>::types();
			}

			T *operator()(Args... args) {
				return func(args...);
			}

		private:
			core::Functor<T *, Args...> func;
	};

	public:
		AssetLoader() {
		}

		template<typename... Args>
		T *operator()(Args... args) const {
			VariadicType types = TypeArray<Args...>::types();
			core::Array<LoaderBase *> valid;
			for(LoaderBase *b : bases) {
				if(b->types() == types) {
					valid += b;
				}
			}
			if(valid.size() > 1) {
				fatal("Unable to load asset : ambiguous argument sequence");
			}
			LoaderFunc<Args...> *ld = dynamic_cast<LoaderFunc<Args...> *>(valid.isEmpty() ? 0 : valid.first());
			if(!ld) {
				fatal("Unable to load asset : no compatible loader found");
			}
			return (*ld)(args...);
		}

		template<typename... Args>
		void addLoader(core::Functor<T *, Args...> f) {
			bases.append(new LoaderFunc<Args...>(f));
		}

		template<typename... Args>
		void addLoader(T *(*f)(Args...)) {
			addLoader(core::Functor<T *, Args...>(f));
		}

		template<typename... Args, typename U>
		void addLoader(U u) {
			addLoader(core::Functor<T *, Args...>(u));
		}

	private:
		core::Array<LoaderBase *> bases;

};

}
}

#endif // N_ASSETS_ASSETLOADER_H
