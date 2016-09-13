/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef YAVE_EVENTHANDLER_H
#define YAVE_EVENTHANDLER_H

#include "yave.h"
#include <y/math/Vec.h>

namespace yave {

class MouseEventHandler {
	public:
		enum MouseButton {
			LeftButton,
			RightButton
		};

		virtual ~MouseEventHandler() {
		}

		virtual void mouse_moved(const math::Vec2i&)						{}
		virtual void mouse_pressed(const math::Vec2i&, MouseButton)			{}
		virtual void mouse_released(const math::Vec2i&, MouseButton)		{}
};


}

#endif // YAVE_EVENTHANDLER_H
