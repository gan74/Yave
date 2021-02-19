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
#ifndef YAVE_GRAPHICS_MEMORY_MEMORYTYPE_H
#define YAVE_GRAPHICS_MEMORY_MEMORYTYPE_H

#include <yave/yave.h>

namespace yave {

enum class MemoryType {
    DontCare = 0,
    DeviceLocal = 1,
    CpuVisible = 2,
    Staging = 3
};

inline constexpr bool is_cpu_visible(MemoryType type) {
    return type == MemoryType::CpuVisible || type == MemoryType::Staging;
}

inline constexpr bool require_staging(MemoryType type) {
    return !is_cpu_visible(type);
}

inline constexpr bool is_memory_type_compatible(MemoryType from, MemoryType to) {
    return to == MemoryType::DontCare || from == to || (is_cpu_visible(from) && is_cpu_visible(to));
}

}

#endif // YAVE_GRAPHICS_MEMORY_MEMORYTYPE_H

