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

#include "EntityIdPool.h"

namespace yave {
namespace ecs {


EntityId::EntityId(index_type index) : _index(index), _version(0) {
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



EntityIdPool::EntityIdPool() {
	_ids.emplace_back();
}

core::Result<void> EntityIdPool::create_with_index(index_type index) {
	y_debug_assert(EntityId(index).is_valid());
	_ids.set_min_capacity(index);
	while(usize(index + 1) >= _ids.size()) {
		_ids.emplace_back();
	}

	if(!_ids[index].is_valid()) {
		_ids[index].set(index);

		if(auto it = std::find(_free.begin(), _free.end(), index); it != _free.end()) {
			_free.erase_unordered(it);
		}
		return core::Ok();
	}

	return core::Err();
}

EntityId EntityIdPool::create() {
	++_size;
	if(!_free.is_empty()) {
		index_type index = _free.pop();
		_ids[index].set(index);
		return _ids[index];
	}

	y_debug_assert(!_ids.is_empty());
	index_type index = index_type(_ids.size() - 1);
	_ids.last().set(index);
	_ids.emplace_back();
	return _ids[index];
}

void EntityIdPool::recycle(EntityId id) {
	_free.push_back(id._index);
	_ids[id._index].clear();
	--_size;
}

EntityIdPool::iterator EntityIdPool::begin() {
	return _ids.begin();
}

EntityIdPool::iterator EntityIdPool::end() {
	y_debug_assert(_ids.last() == EntityId());
	return _ids.end() - 1;
}

EntityIdPool::const_iterator EntityIdPool::begin() const {
	return _ids.begin();
}

EntityIdPool::const_iterator EntityIdPool::end() const {
	y_debug_assert(_ids.last() == EntityId());
	return _ids.end() - 1;
}

usize EntityIdPool::size() const {
	return _size;
}

}
}
