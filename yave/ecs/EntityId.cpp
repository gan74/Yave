/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "EntityId.h"

namespace yave {
namespace ecs {

EntityId EntityId::from_unversioned_index(index_type index) {
	EntityId id;
	id._index = index;
	id._version = 0;
	return id;
}

void EntityId::clear() {
	y_debug_assert(is_valid());
	y_debug_assert(_version != invalid_index);
	_index = invalid_index;
}

void EntityId::set(index_type index) {
	y_debug_assert(!is_valid());
	_index = index;
	++_version;
}

EntityId::index_type EntityId::index() const {
	return _index;
}

bool EntityId::is_valid() const {
	return _index != invalid_index;
}

bool EntityId::operator==(const EntityId& other) const {
	return std::tie(_index, _version) == std::tie(other._index, other._version);
}

bool EntityId::operator!=(const EntityId& other) const {
	return std::tie(_index, _version) != std::tie(other._index, other._version);
}


}
}
