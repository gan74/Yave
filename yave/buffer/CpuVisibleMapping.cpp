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

#include "CpuVisibleMapping.h"

#include <yave/Device.h>

namespace yave {

CpuVisibleMapping::CpuVisibleMapping() : DeviceLinked(), _memory(VK_NULL_HANDLE), _size(0), _mapping(nullptr) {
}

CpuVisibleMapping::CpuVisibleMapping(BufferBase* base) :
		DeviceLinked(base->get_device()),
		_memory(base->get_vk_device_memory()),
		_size(base->byte_size()),
		_mapping(get_device()->get_vk_device().mapMemory(_memory, 0, _size)) {
}

CpuVisibleMapping::CpuVisibleMapping(CpuVisibleMapping&& other) : CpuVisibleMapping() {
	swap(other);
}

CpuVisibleMapping& CpuVisibleMapping::operator=(CpuVisibleMapping&& other) {
	swap(other);
	return *this;
}

CpuVisibleMapping::~CpuVisibleMapping() {
	if(get_device() && _mapping) {
		get_device()->get_vk_device().unmapMemory(_memory);
	}
}

usize CpuVisibleMapping::byte_size() const {
	return _size;
}

void CpuVisibleMapping::swap(CpuVisibleMapping& other) {
	std::swap(_mapping, other._mapping);
	std::swap(_memory, other._memory);
	std::swap(_size, other._size);
}

void* CpuVisibleMapping::data() {
	return _mapping;
}

const void* CpuVisibleMapping::data() const {
	return _mapping;
}



}
