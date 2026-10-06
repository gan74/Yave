/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#include "BlueprintInstance.h"

#include <y/utils/memory.h>

#include <algorithm>
#include <cstring>

namespace yave {


void* BlueprintStorage::alloc(usize size, usize alignment) {
    y_debug_assert(alignment && alignment <= alignof(std::max_align_t));

    _offset = align_up_to(_offset, alignment);
    if(_chunks.is_empty() || _offset + size > _chunks.last().size()) {
        _chunks.emplace_back(std::max(size, storage_chunk_size));
        _offset = 0;
    }

    void* ptr = _chunks.last().data() + _offset;
    _offset += size;
    return ptr;
}


core::Span<BlueprintInstance::EntryPoint> BlueprintInstance::entry_points() const {
    return _entry_points;
}

core::Result<void, BlueprintError> BlueprintInstance::trigger(const EntryPoint& entry_point, const void* payload) noexcept {
    y_profile();

    std::memcpy(entry_point.payload, payload, entry_point.payload_size);

    for(const BlueprintInstruction& instruction : entry_point.instructions) {
        try {
            instruction.func();
        } catch(const std::exception& e) {
            return core::Err(BlueprintError{instruction.node,core::String(e.what())});
        }
    }

    return core::Ok();
}

}
