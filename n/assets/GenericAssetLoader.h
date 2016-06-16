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

#ifndef N_ASSETS_GENERICASSETLOADER_H
#define N_ASSETS_GENERICASSETLOADER_H

#include "AssetManager.h"

namespace n {
namespace assets {

template<typename T, bool DefaultAsync = true>
class GenericAssetLoader
{
	public:
		using LoadedType = typename T::AssetType;

		template<typename U, typename... Args>
		class AssetReader
		{
			struct Runner
			{
				Runner() {
					getLoader()->addReader<Args...>(U());
				}
			};

			public:
				AssetReader() {
					n::unused(runner);
				}

				virtual LoadedType *operator()(Args...) = 0;

			private:
				static Runner runner;
		};

		template<typename... Args>
		static T load(Args... args, bool async = DefaultAsync)  {
			GenericAssetLoader<T> *ld = getLoader();
			return async ? T(ld->asyncBuffer.load(args...)) : T(ld->immediateBuffer.load(args...));
		}

		template<typename... Args, typename U>
		void addReader(const U &t) {
			asyncBuffer.template addLoader<Args...>(t);
			immediateBuffer.template addLoader<Args...>(t);
		}

		void gc() {
			asyncBuffer.gc();
			immediateBuffer.gc();
		}

		static GenericAssetLoader<T> *getLoader() {
			static GenericAssetLoader<T> *loader = 0;
			if(!loader) {
				loader = new GenericAssetLoader<T>();
			}
			return loader;
		}

	protected:
		GenericAssetLoader() : asyncBuffer(buffer), immediateBuffer(buffer) {
		}

	private:
		template<typename U, bool D>
		friend class GenericAssetLoader;

		AssetBuffer<LoadedType> buffer;
		AssetManager<LoadedType, AsyncLoadingPolicy<LoadedType>> asyncBuffer;
		AssetManager<LoadedType, ImmediateLoadingPolicy<LoadedType>> immediateBuffer;

};

template<typename T, bool D>
template<typename U, typename... Args>
typename GenericAssetLoader<T, D>::template AssetReader<U, Args...>::Runner
GenericAssetLoader<T, D>::AssetReader<U, Args...>::runner =
typename GenericAssetLoader<T, D>::template AssetReader<U, Args...>::Runner();

}
}

#endif // N_ASSETS_GENERICASSETLOADER_H

