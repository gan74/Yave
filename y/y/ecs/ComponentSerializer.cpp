/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "ComponentSerializer.h"

namespace y {
namespace ecs {

ComponentInfoSerializerBase::~ComponentInfoSerializerBase() {
}

ComponentSerializerBase::~ComponentSerializerBase() {
}

ComponentSerializerBase::ComponentSerializerBase(Archetype* arc) : _archetype(arc) {
}

ComponentSerializerWrapper::ComponentSerializerWrapper(std::unique_ptr<ComponentSerializerBase> ptr) : _storage(std::move(ptr)), _serializer(*_storage.get()) {
}

void ComponentListSerializer::add(ComponentSerializerWrapper wrapper) {
	_wrappers.emplace_back(std::move(wrapper));
}

core::MutableSpan<ComponentSerializerWrapper> ComponentListSerializer::wrappers() {
	return _wrappers;
}

core::Span<ComponentSerializerWrapper> ComponentListSerializer::wrappers() const {
	return _wrappers;
}


}
}
