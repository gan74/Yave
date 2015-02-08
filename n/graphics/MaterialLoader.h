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

#ifndef N_GRAPHICS_MATERIALLOADER
#define N_GRAPHICS_MATERIALLOADER

#include "Material.h"
#include <n/assets/Asset.h>
#include <n/assets/AssetBuffer.h>

namespace n {
namespace graphics {

class MaterialLoader
{
	static MaterialLoader *loader;

	public:
		template<typename T, typename... Args>
		class MaterialDecoder
		{
			struct Runner
			{
				Runner() {
					getLoader()->addDec<Args...>(T());
				}
			};

			public:
				MaterialDecoder() {
					n::unused(runner);
				}

				virtual internal::Material<> *operator()(Args...) = 0;

			private:
				static Runner runner;
		};

		template<typename... Args>
		static Material<> load(Args... args, bool async = true)  {
			MaterialLoader *ld = getLoader();
			return async ? Material<>(ld->asyncBuffer.load(args...)) : Material<>(ld->immediateBuffer.load(args...));
		}

		template<typename... Args, typename T>
		void addDec(const T &t) {
			asyncBuffer.addLoader<Args...>(t);
			immediateBuffer.addLoader<Args...>(t);
		}

	private:
		static MaterialLoader *getLoader() {
			if(!loader) {
				loader = new MaterialLoader();
			}
			return loader;
		}

		MaterialLoader() {
		}

		assets::AssetBuffer<internal::Material<>, assets::AsyncLoadingPolicy<internal::Material<>>> asyncBuffer;
		assets::AssetBuffer<internal::Material<>, assets::ImmediateLoadingPolicy<internal::Material<>>> immediateBuffer;

};

template<typename T, typename... Args>
typename MaterialLoader::MaterialDecoder<T, Args...>::Runner MaterialLoader::MaterialDecoder<T, Args...>::runner = MaterialLoader::MaterialDecoder<T, Args...>::Runner();


}
}


#endif // MATERIALLOADER

