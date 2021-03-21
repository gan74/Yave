/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "DeviceMemoryView.h"
#include "DeviceMemoryHeap.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>

namespace yave {

DeviceMemoryView::DeviceMemoryView(const DeviceMemory& mem) :
        _heap(mem.heap()),
        _memory(mem.vk_memory()),
        _offset(mem.vk_offset()) {
}

VkMappedMemoryRange DeviceMemoryView::vk_mapped_range(usize size, usize offset) const {
    const usize atom_size = device_properties().non_coherent_atom_size;

    const usize full_offset = _offset + offset;
    const usize aligned_offset = memory::align_down_to(full_offset, atom_size);
    y_debug_assert(aligned_offset <= full_offset);
    const usize end = full_offset + size;
    y_debug_assert(end > aligned_offset);
    const usize aligned_size = memory::align_up_to(end - aligned_offset, atom_size);
    y_debug_assert(end > full_offset);

    VkMappedMemoryRange range = vk_struct();
    {
        range.memory = _memory;
        range.offset = aligned_offset;
        range.size = aligned_size;
    }
    return range;
}

VkDeviceMemory DeviceMemoryView::vk_memory() const {
    return _memory;
}

usize DeviceMemoryView::vk_offset() const {
    return _offset;
}

void* DeviceMemoryView::map() {
    return _heap->map(*this);
}

void DeviceMemoryView::unmap() {
    _heap->unmap(*this);
}

}

