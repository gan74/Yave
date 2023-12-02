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

#include "format.h"

#include <memory>

namespace y {
namespace detail {

static constexpr usize fmt_block_size = 1024;
static constexpr usize fmt_buffer_count = 4;

static constexpr usize fmt_total_buffer_size = fmt_block_size * fmt_buffer_count;

static thread_local std::unique_ptr<char[]> fmt_buffer = nullptr;
static thread_local usize fmt_start_index = 0;

core::MutableSpan<char> alloc_fmt_buffer() {
    if(!fmt_buffer) {
        fmt_buffer = std::make_unique<char[]>(fmt_total_buffer_size + 1);
        fmt_buffer[fmt_total_buffer_size] = 0;
    }

    if(fmt_start_index + fmt_block_size > fmt_total_buffer_size) {
        fmt_start_index = 0;
    }

    const usize index = fmt_start_index;
    fmt_start_index += fmt_block_size;

    return core::MutableSpan<char>(&fmt_buffer[index], fmt_block_size);
}
}

}

