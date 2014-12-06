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

	using ArgumentTypes = core::Array<Type>;

	template<typename... Ar>
	class TypeArray
	{
		template<typename W, typename... A>
		struct TypeHelper
		{
			static ArgumentTypes args() {
				return ArgumentTypes(typeid(W), TypeHelper<A...>::args());
			}
		};

		template<typename W, typename Z>
		struct TypeHelper<W, Z>
		{
			static ArgumentTypes args() {
				return ArgumentTypes(typeid(W), typeid(Z));
			}
		};

		public:
			static ArgumentTypes args() {
				ArgumentTypes arr = TypeHelper<Ar..., NullType, NullType>::args();
				arr.pop();
				arr.pop();
				return arr;
			}
	};

	class LoaderBase
	{
		public:
			virtual ArgumentTypes args() const = 0;
	};

	template<typename... Args>
	class LoaderFunc : public LoaderBase
	{
		public:
			template<typename U>
			LoaderFunc(U u) : func(u) {
			}

			ArgumentTypes args() const override {
				return TypeArray<Args...>::args();
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
			ArgumentTypes argTypes = TypeArray<Args...>::args();
			core::Array<LoaderBase *> valid;
			for(LoaderBase *b : bases) {
				if(b->args() == argTypes) {
					valid += b;
				}
			}
			if(valid.size() > 1) {
				fatal("Unable to load asset : ambiguous argument sequence : " + argsToString(argTypes) + " has " + valid.size() + " possible solutions.");
			}
			LoaderFunc<Args...> *ld = dynamic_cast<LoaderFunc<Args...> *>(valid.isEmpty() ? 0 : valid.first());
			if(!ld) {
				fatal("Unable to load asset : no compatible loader found for " + argsToString(argTypes) + ", candidates are : " + AsCollection(bases.mapped([](const LoaderBase *b) {
						return "\n  " + argsToString(b->args());
					})).make(core::String(" or ")));
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
		static core::String argsToString(const ArgumentTypes &types) {
			return "{" + AsCollection(types.mapped([](const Type &t) {
						return t.name();
					})).make(core::String(", ")) + "}";
		}

		core::Array<LoaderBase *> bases;

};

}
}

#endif // N_ASSETS_ASSETLOADER_H
