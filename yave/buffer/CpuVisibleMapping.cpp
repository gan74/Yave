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

CpuVisibleMapping::CpuVisibleMapping() : _mapping(nullptr) {
}

CpuVisibleMapping::CpuVisibleMapping(const SubBufferBase& buff) :
		_buffer(buff),
		_mapping(_buffer.get_device()->get_vk_device().mapMemory(_buffer.get_vk_device_memory(), 0, _buffer.byte_size())) {
#ifdef YAVE_DEBUG_BUFFERS
	_buffer->_mapped++;
#endif
}

CpuVisibleMapping::CpuVisibleMapping(CpuVisibleMapping&& other) : CpuVisibleMapping() {
	swap(other);
}

CpuVisibleMapping& CpuVisibleMapping::operator=(CpuVisibleMapping&& other) {
	swap(other);
	return *this;
}

CpuVisibleMapping::~CpuVisibleMapping() {
#ifdef YAVE_DEBUG_BUFFERS
	if(_buffer) {
		_buffer->_mapped--;
	}
#endif
	if(_buffer.get_device() && _mapping) {
		_buffer.get_device()->get_vk_device().unmapMemory(_buffer.get_vk_device_memory());
	}
}

usize CpuVisibleMapping::byte_size() const {
	return _buffer.byte_size();
}

void CpuVisibleMapping::swap(CpuVisibleMapping& other) {
	std::swap(_mapping, other._mapping);
	std::swap(_buffer, other._buffer);
}

void* CpuVisibleMapping::data() {
	return _mapping;
}

const void* CpuVisibleMapping::data() const {
	return _mapping;
}



}
