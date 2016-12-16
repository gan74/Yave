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
		_mapping(_buffer.device()->vk_device().mapMemory(_buffer.vk_device_memory(), 0, _buffer.byte_size())) {
}

CpuVisibleMapping::CpuVisibleMapping(CpuVisibleMapping&& other) : CpuVisibleMapping() {
	swap(other);
}

CpuVisibleMapping& CpuVisibleMapping::operator=(CpuVisibleMapping&& other) {
	swap(other);
	return *this;
}

CpuVisibleMapping::~CpuVisibleMapping() {
	if(_buffer.device() && _mapping) {
		_buffer.device()->vk_device().unmapMemory(_buffer.vk_device_memory());
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
