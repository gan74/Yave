/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "MaterialDrawData.h"

#include <yave/graphics/device/MaterialAllocator.h>

namespace yave {

MaterialDrawData::MaterialDrawData(MaterialDrawData&& other) {
    swap(other);
}

MaterialDrawData& MaterialDrawData::operator=(MaterialDrawData&& other) {
    swap(other);
    return *this;
}

MaterialDrawData::~MaterialDrawData() {
    y_debug_assert(is_null());
}

void MaterialDrawData::swap(MaterialDrawData& other) {
    std::swap(_index, other._index);
    std::swap(_textures, other._textures);
    std::swap(_parent, other._parent);
}

bool MaterialDrawData::is_null() const {
    return _index == u32(-1);
}

u32 MaterialDrawData::index() const {
    return _index;
}

void MaterialDrawData::recycle() {
    _parent->recycle(this);
}

}
