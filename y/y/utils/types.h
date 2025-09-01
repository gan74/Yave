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
#ifndef Y_UTILS_TYPES_H
#define Y_UTILS_TYPES_H

#include <cstdint>
#include <cstddef>

#include <utility>

namespace y {

struct NonCopyable {
    inline constexpr NonCopyable() {}

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;

    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

struct NonMovable : NonCopyable {
    inline constexpr NonMovable() {}

    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = std::make_unsigned_t<std::size_t>;
using isize = std::make_signed_t<std::size_t>;


template<typename T>
using Owner = T;

template<typename T>
using NotOwner = T;


namespace detail {
enum Enum { _ = u32(-1) };
}

using uenum = std::underlying_type<detail::Enum>::type;

inline constexpr usize operator"" _uu(unsigned long long int t) {
    return usize(t);
}

template<typename... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template<typename T>
class Uninitialized : NonMovable {
    public:
        Uninitialized() = default;

        ~Uninitialized() {
            y_debug_assert(!_is_init);
        }

        template<typename... Args>
        T& init(Args&&... args) {
            y_debug_assert(toggle_init());
            return *(new(&_storage.obj) T(y_fwd(args)...));
        }

        void destroy() {
            y_debug_assert(_is_init);
            _storage.obj.~T();
            y_debug_assert(!toggle_init());
        }

        T* operator->() {
            y_debug_assert(_is_init);
            return &_storage.obj;
        }

        const T* operator->() const {
            y_debug_assert(_is_init);
            return &_storage.obj;
        }

        T& operator*() {
            y_debug_assert(_is_init);
            return _storage.obj;
        }

        const T& operator*() const {
            y_debug_assert(_is_init);
            return _storage.obj;
        }

    private:
        union Storage {
            Storage() : dummy(0) {
            }

            ~Storage() {
            }

            T obj;
            u8 dummy;
        } _storage;

#ifdef Y_DEBUG
        bool _is_init = false;
        bool toggle_init() {
            return _is_init = !_is_init;
        }
#endif
};

}


#endif // Y_UTILS_TYPES_H

