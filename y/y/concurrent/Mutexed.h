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

#ifndef Y_CONCURRENT_MUTEXED_H
#define Y_CONCURRENT_MUTEXED_H

#include <y/utils.h>

#include <mutex>
#include <shared_mutex>

namespace y {
namespace concurrent {

class AssertLock : NonMovable {
    public:
        void lock() {
            y_debug_assert(!_locked.exchange(true));
        }

        void unlock() {
            y_debug_assert(_locked.exchange(false));
        }

    private:
        std::atomic<bool> _locked = false;
};


template<typename T, typename Lock = std::mutex>
class Mutexed : NonMovable {
    public:
        template<typename... Args>
        Mutexed(Args&&... args) : _obj(y_fwd(args)...) {
        }

        template<typename F>
        inline decltype(auto) locked(F&& func) {
            const std::unique_lock lock(_lock);
            return func(_obj);
        }

        template<typename F>
        inline decltype(auto) locked(F&& func) const {
            const std::unique_lock lock(_lock);
            return func(_obj);
        }

        template<typename F>
        inline decltype(auto) locked_shared(F&& func) const {
            const std::shared_lock lock(_lock);
            return func(_obj);
        }

    private:
        T _obj;
        [[no_unique_address]] mutable Lock _lock;
};


}
}

#endif // Y_CONCURRENT_MUTEXED_H

