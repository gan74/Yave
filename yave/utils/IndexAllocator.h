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
#ifndef YAVE_UTILS_INDEXALLOCATOR_H
#define YAVE_UTILS_INDEXALLOCATOR_H

#include <yave/yave.h>

#include <y/core/Vector.h>

namespace yave {

template<typename I = u32>
class IndexAllocator {
    public:
        using index_type = I;

        IndexAllocator() = default;

        inline usize size() const {
            return _max - _free.size();
        }

        inline usize max_index() const {
            return _max + 1;
        }

        inline index_type alloc() {
            if(_free.is_empty()) {
                return index_type(_max++);
            }

            return _free.pop();
        }

        inline void free(index_type index) {
            y_debug_assert(std::find(_free.begin(), _free.end(), index) == _free.end());
            _free << index;
        }

    private:
        core::Vector<index_type> _free;
        usize _max = 0;
};

}

#endif // YAVE_UTILS_INDEXALLOCATOR_H

