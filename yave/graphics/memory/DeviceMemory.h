/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H

#include <yave/graphics/graphics.h>

namespace yave {

class DeviceMemory final : NonCopyable {
    public:
        DeviceMemory() = default;

        DeviceMemory(VmaAllocation alloc) : _alloc(alloc) {
        }

        ~DeviceMemory() {
            y_debug_assert(!_alloc);
        }

        DeviceMemory(DeviceMemory&& other) {
            swap(other);
        }

        DeviceMemory& operator=(DeviceMemory&& other) {
            swap(other);
            return *this;
        }

        bool is_null() const {
            return !_alloc;
        }

    private:
        friend class LifetimeManager;
        friend class DeviceMemoryView;

        void free() {
            y_debug_assert(_alloc);
            vmaFreeMemory(device_allocator(), _alloc);
            _alloc = {};
        }

        void swap(DeviceMemory& other) {
            std::swap(_alloc, other._alloc);
        }

        VmaAllocation _alloc = {};
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H

