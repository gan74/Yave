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

#ifdef N_USE_SDL_IMAGE

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <n/concurrent/Mutex.h>
#include <n/utils.h>
#include <n/defines.h>
#include "ImageLoader.h"
#include <iostream>

namespace n {
namespace graphics {

class SDLImageDecoder : public graphics::ImageLoader::ImageDecoder<SDLImageDecoder, core::String>
{
	public:
		SDLImageDecoder() : graphics::ImageLoader::ImageDecoder<SDLImageDecoder, core::String>() {
		}

		graphics::internal::Image *operator()(core::String name) override {
			static concurrent::Mutex lock;
			lock.lock();
			SDL_Surface *surf = IMG_Load(name.toChar());
			if(!surf) {
				std::cerr<<"Failed to load \""<<name<<"\" : "<<IMG_GetError()<<std::endl;
				lock.unlock();
				return 0;
			}
			SDL_LockSurface(surf);
			math::Vec2ui size(surf->w, surf->h);
			SDL_PixelFormat frm;
			frm.format = SDL_PIXELFORMAT_RGBA8888;
			frm.BitsPerPixel = 32;
			frm.BytesPerPixel = 4;
			frm.Rmask = 0x000000FF;
			frm.Gmask = 0x0000FF00;
			frm.Bmask = 0x00FF0000;
			frm.Amask = 0xFF000000;

			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				std::swap(frm.Amask, frm.Rmask);
				std::swap(frm.Bmask, frm.Gmask);
			}

			SDL_Surface* cs = SDL_ConvertSurface(surf, &frm, SDL_SWSURFACE);
			SDL_LockSurface(cs);
			void *dat = cs->pixels;
			cs->pixels = 0;
			SDL_UnlockSurface(surf);
			SDL_UnlockSurface(cs);
			lock.unlock();
			return new internal::Image(size, ImageFormat::RGBA8, dat);
		}
};

}
}

#endif

