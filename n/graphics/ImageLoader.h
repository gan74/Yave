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

#ifndef N_GRAPHICS_IMAGELOADER_H
#define N_GRAPHICS_IMAGELOADER_H

#include "Image.h"
#include <n/assets/AssetManager.h>

namespace n {
namespace graphics {

class ImageLoader
{
	static ImageLoader *loader;
	public:
		template<typename T, typename... Args>
		class ImageDecoder
		{
			struct Runner
			{
				Runner() {
					getLoader()->addDec<Args...>(T());
				}
			};

			public:
				ImageDecoder() {
					n::unused(runner);
				}

				virtual internal::Image *operator()(Args...) = 0;

			private:
				static Runner runner;
		};

		template<typename... Args>
		static Image load(Args... args, bool async = true)  {
			ImageLoader *ld = getLoader();
			return async ? Image(ld->asyncBuffer.load(args...)) : Image(ld->immediateBuffer.load(args...));
		}

		template<typename... Args, typename T>
		void addDec(const T &t) {
			asyncBuffer.addLoader<Args...>(t);
			immediateBuffer.addLoader<Args...>(t);
		}

	private:
		static ImageLoader *getLoader() {
			if(!loader) {
				loader = new ImageLoader();
			}
			return loader;
		}

		ImageLoader() : asyncBuffer(buffer), immediateBuffer(buffer) {
		}


		assets::AssetBuffer<internal::Image> buffer;
		assets::AssetManager<internal::Image, assets::AsyncLoadingPolicy<internal::Image>> asyncBuffer;
		assets::AssetManager<internal::Image, assets::ImmediateLoadingPolicy<internal::Image>> immediateBuffer;
};


template<typename T, typename... Args>
typename ImageLoader::ImageDecoder<T, Args...>::Runner ImageLoader::ImageDecoder<T, Args...>::runner = ImageLoader::ImageDecoder<T,Args...>::Runner();

}
}

#endif // N_GRAPHICS_IMAGELOADER_H
