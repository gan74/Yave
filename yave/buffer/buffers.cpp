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

#include "buffers.h"

namespace yave {

template<typename T>
auto to_vk_mem_flags(T t) {
	return vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits(t));
}

bool is_cpu_visible(MemoryFlags flags) {
	return (to_vk_mem_flags(vk::MemoryPropertyFlagBits::eHostVisible) & to_vk_mem_flags(flags)) == to_vk_mem_flags(vk::MemoryPropertyFlagBits::eHostVisible);
}

}
