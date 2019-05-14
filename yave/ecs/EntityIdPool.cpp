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


EntityId2::EntityId2(u32 index) : _index(index), _version(0) {
}

void EntityId2::clear() {
	y_debug_assert(_index != invalid_index);
	y_debug_assert(_version != invalid_index);
	_index = invalid_index;
	++_version;
}

void EntityId2::set(u32 index) {
	y_debug_assert(_index == invalid_index);
	_index = index;
}

usize EntityId2::index() const {
	return _index;
}


EntityIdPool::EntityIdPool() {
	_ids.emplace_back();
}

EntityId2 EntityIdPool::create() {
	++_size;
	if(!_free.is_empty()) {
		u32 index = _free.pop();
		_ids[index].set(index);
		return _ids[index];
	}

	u32 index = u32(_ids.size());
	return _ids.emplace_back(index);
}

void EntityIdPool::recycle(EntityId2 id) {
	_free.push_back(id._index);
	_ids[id._index].clear();
	--_size;
}

EntityIdPool::iterator EntityIdPool::begin() {
	return _ids.begin();
}

EntityIdPool::iterator EntityIdPool::end() {
	return _ids.end() - 1;
}

EntityIdPool::const_iterator EntityIdPool::begin() const {
	return _ids.begin();
}

EntityIdPool::const_iterator EntityIdPool::end() const {
	return _ids.end() - 1;
}

usize EntityIdPool::size() const {
	return _size;
}

}
}
