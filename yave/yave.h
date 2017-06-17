/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_H
#define YAVE_H

#include <y/utils.h>

#include <y/math/Vec.h>
#include <y/math/math.h>
#include <y/math/Matrix.h>

#include <y/core/Ptr.h>
#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/core/ArrayProxy.h>

#ifdef Y_OS_WIN
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.hpp>

namespace yave {

using namespace y;

static_assert(math::Matrix4<>::is_column_major, "Yave requires column-major matrices");

struct Version {
	u32 patch : 12;
	u32 minor : 10;
	u32 major : 10;
};


class Device;

using DevicePtr = const Device*;



namespace detail {
template<typename... Args>
constexpr int max(Args... args) {
	int max = 0;
	// https://www.youtube.com/watch?v=nhk8pF_SlTk
	unused(std::initializer_list<int>{ (max = std::max(max, args))... });
	return max;
}
}

template<typename T, typename U, typename... Args>
inline DevicePtr common_device(T&& t, U&& u, Args&&... args) {
	DevicePtr l = t.device();
	DevicePtr r = common_device(std::forward<U>(u), std::forward<Args>(args)...);
	if(l && r && l != r) {
		fatal("Objects have different devices.");
	}
	return l ? l : r;
}

template<typename T>
inline DevicePtr common_device(T&& t) {
	return t.device();
}



template<typename T>
using is_safe_base = bool_type<!std::is_default_constructible_v<T> &&
							   !std::is_copy_constructible_v<T> &&
							   !std::is_copy_assignable_v<T> &&
							   !std::is_move_constructible_v<T>>;
}

#endif // YAVE_H
