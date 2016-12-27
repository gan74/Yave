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
#ifndef YAVE_H
#define YAVE_H

#include <y/utils.h>

#include <y/math/Vec.h>
#include <y/math/math.h>
#include <y/math/Matrix.h>

#include <y/core/Ptr.h>
#include <y/core/Vector.h>
#include <y/core/String.h>

#ifdef Y_OS_WIN
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.hpp>

namespace yave {

using namespace y;

struct Version {
	u32 patch : 12;
	u32 minor : 10;
	u32 major : 10;
};


class Device;

using DevicePtr = const Device*;


template<typename T, typename U, typename... Args>
inline DevicePtr common_device(T&& t, U&& u, Args&&... args) {
	DevicePtr l = t.device();
	DevicePtr r = common_device(std::forward<U>(u), std::forward<Args>(args)...);
	if(l && r && l != r) {
		fatal("Objects have different devices");
	}
	return l ? l : r;
}

template<typename T>
inline DevicePtr common_device(T&& t) {
	return t.device();
}


}

#endif // YAVE_H
