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




template<typename... Args>
struct type_pack {
    static constexpr usize size = sizeof...(Args);
};

namespace detail {
template<typename T, typename U, typename... Args>
constexpr usize type_index_in_pack() {
    if constexpr(std::is_same_v<T, U>) {
        return 0;
    } else {
        static_assert(sizeof...(Args) > 0, "Type is not present in pack");
        return type_index_in_pack<T, Args...>() + 1;
    }
}
}

template<typename... Args, typename... More>
constexpr auto concatenate_packs(type_pack<Args...>, type_pack<More...>) {
    return type_pack<Args..., More...>{};
}

template<typename T, typename... Args>
consteval usize type_index_in_pack(type_pack<Args...> pack) {
    static_assert(pack.size > 0, "type_pack is empty");
    return detail::type_index_in_pack<T, Args...>();

}

}


#endif // Y_UTILS_TYPES_H

