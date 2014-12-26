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
#include <n/assets/AssetBuffer.h>

namespace n {
namespace graphics {

class ImageLoader
{
	static ImageLoader *loader;
	public:
		template<typename T>
		class ImageDecoder
		{
			struct Runner
			{
				Runner() {
					getLoader()->addCodec(T());
				}
			};

			public:
				ImageDecoder() {
					n::unused(runner);
				}

				virtual ImageInternal *operator()(const core::String &n) = 0;

			private:
				static Runner runner;
		};


		static Image load(const core::String &filename)  {
			ImageLoader *ld = getLoader();
			return Image(ld->asyncBuffer.load(filename));
		}


		template<typename T>
		void addCodec(const T &t) {
			asyncBuffer.addLoader<core::String>(t);
			imediateBuffer.addLoader<core::String>(t);
		}

	private:
		static ImageLoader *getLoader() {
			if(!loader) {
				loader = new ImageLoader();
			}
			return loader;
		}

		ImageLoader() {
		}

		assets::AssetBuffer<ImageInternal, assets::AsyncLoadingPolicy<ImageInternal>> asyncBuffer;
		assets::AssetBuffer<ImageInternal, assets::ImediateLoadingPolicy<ImageInternal>> imediateBuffer;
};


template<typename T>
typename ImageLoader::ImageDecoder<T>::Runner ImageLoader::ImageDecoder<T>::runner = ImageLoader::ImageDecoder<T>::Runner();

}
}

#endif // N_GRAPHICS_IMAGELOADER_H
