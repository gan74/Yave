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
#ifndef Y_UTILS_MEMORY_H
#define Y_UTILS_MEMORY_H

#include <y/utils.h>

#include <bit>

namespace y {

constexpr usize max_alignment = std::alignment_of<std::max_align_t>::value;

template<typename U>
constexpr U align_up_to(U value, U alignment) {
    if(const U diff = value % alignment) {
        return value + alignment - diff;
    }
    return value;
}

template<typename U>
constexpr U align_down_to(U value, U alignment) {
    const U diff = value % alignment;
    return value - diff;
}

template<typename U>
constexpr U align_up_to_max(U size) {
    return align_up_to(size, U(max_alignment));
}

}

#endif // Y_UTILS_MEMORY_H

