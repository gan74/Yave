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

#ifndef N_GRAPHICS_RENDERER
#define N_GRAPHICS_RENDERER

#include <n/utils.h>
#include <n/signals/Signal.h>
#include "GLContext.h"

namespace n {
namespace graphics {

class Renderer : core::NonCopyable
{
	public:
		Renderer() {
		}

		virtual ~Renderer() {
		}

		void operator()() {
			render(prepare());
		}

	public:
		virtual void *prepare() = 0;
		virtual void render(void *ptr) = 0;

};

}
}

#endif // N_GRAPHICS_RENDERER

