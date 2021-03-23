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

#ifndef Y_CONCURRENT_VECTORLOCK_H
#define Y_CONCURRENT_VECTORLOCK_H

#include <y/core/Vector.h>

namespace y {
namespace concurrent {

template<typename T>
class VectorLock : NonCopyable {
    public:
        VectorLock(VectorLock&&) = default;
        VectorLock& operator=(VectorLock&&) = default;

        VectorLock(core::Vector<std::reference_wrapper<T>> locks) : _locks(std::move(locks)) {
            while(!try_lock_all()) {
            }
        }

        ~VectorLock() {
            for(usize i = 0; i != _locks.size(); ++i) {
                _locks[_locks.size() - i - 1].get().unlock();
            }
        }

    private:
        bool try_lock_all() {
            for(usize i = 0; i != _locks.size(); ++i) {
                if(!_locks[i].get().try_lock()) {
                    for(usize j = 0; j + 1 < i; ++j) {
                        _locks[i - j - 1].get().unlock();
                    }
                    return false;
                }
            }
            return true;
        }

        core::Vector<std::reference_wrapper<T>> _locks;
};


}
}

#endif // Y_CONCURRENT_VECTORLOCK_H

