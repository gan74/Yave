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

#ifndef N_GRAPHICS_MESHLOADER
#define N_GRAPHICS_MESHLOADER

#include "MeshInstance.h"
#include <n/assets/Asset.h>
#include <n/assets/AssetManager.h>

namespace n {
namespace graphics {

class MeshLoader
{
	public:
		template<typename T, typename... Args>
		class MeshReader
		{
			struct Runner
			{
				Runner() {
					getLoader()->addReader<Args...>(T());
				}
			};

			public:
				MeshReader() {
					n::unused(runner);
				}

				virtual internal::MeshInstance<> *operator()(Args...) = 0;

			private:
				static Runner runner;
		};

		template<typename... Args>
		static MeshInstance<> load(Args... args, bool async = true)  {
			MeshLoader *ld = getLoader();
			return async ? MeshInstance<>(ld->asyncBuffer.load(args...)) : MeshInstance<>(ld->immediateBuffer.load(args...));
		}

		template<typename... Args, typename T>
		void addReader(const T &t) {
			asyncBuffer.addLoader<Args...>(t);
			immediateBuffer.addLoader<Args...>(t);
		}

	private:
		static MeshLoader *getLoader() {
			static MeshLoader *loader = 0;
			if(!loader) {
				loader = new MeshLoader();
			}
			return loader;
		}

		MeshLoader() : asyncBuffer(buffer), immediateBuffer(buffer) {
		}

		assets::AssetBuffer<internal::MeshInstance<>> buffer;
		assets::AssetManager<internal::MeshInstance<>, assets::AsyncLoadingPolicy<internal::MeshInstance<>>> asyncBuffer;
		assets::AssetManager<internal::MeshInstance<>, assets::ImmediateLoadingPolicy<internal::MeshInstance<>>> immediateBuffer;

};

template<typename T, typename... Args>
typename MeshLoader::MeshReader<T, Args...>::Runner MeshLoader::MeshReader<T, Args...>::runner = MeshLoader::MeshReader<T, Args...>::Runner();


}
}


#endif // N_GRAPHICS_MESHLOADER

