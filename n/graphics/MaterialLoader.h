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
#include <n/assets/AssetManager.h>

namespace n {
namespace graphics {

class MaterialLoader
{
	public:
		template<typename T, typename... Args>
		class MaterialReader
		{
			struct Runner
			{
				Runner() {
					getLoader()->addReader<Args...>(T());
				}
			};

			public:
				MaterialReader() {
					n::unused(runner);
				}

				virtual MaterialData *operator()(Args...) = 0;

			private:
				static Runner runner;
		};

		template<typename... Args>
		static Material load(Args... args, bool async = true)  {
			MaterialLoader *ld = getLoader();
			return async ? Material(ld->asyncBuffer.load(args...)) : Material(ld->immediateBuffer.load(args...));
		}

		template<typename... Args, typename T>
		void addReader(const T &t) {
			asyncBuffer.addLoader<Args...>(t);
			immediateBuffer.addLoader<Args...>(t);
		}

	private:
		static MaterialLoader *getLoader() {
			static MaterialLoader *loader = 0;
			if(!loader) {
				loader = new MaterialLoader();
			}
			return loader;
		}

		MaterialLoader() : asyncBuffer(buffer), immediateBuffer(buffer) {
		}

		assets::AssetBuffer<MaterialData> buffer;
		assets::AssetManager<MaterialData, assets::AsyncLoadingPolicy<MaterialData>> asyncBuffer;
		assets::AssetManager<MaterialData, assets::ImmediateLoadingPolicy<MaterialData>> immediateBuffer;

};

template<typename T, typename... Args>
typename MaterialLoader::MaterialReader<T, Args...>::Runner MaterialLoader::MaterialReader<T, Args...>::runner = MaterialLoader::MaterialReader<T, Args...>::Runner();

}
}


#endif // MATERIALLOADER

