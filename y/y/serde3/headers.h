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
#ifndef Y_SERDE3_HEADERS_H
#define Y_SERDE3_HEADERS_H

#include <y/utils/name.h>
#include <y/utils/hash.h>

#include "traits.h"
#include "poly.h"


#define Y_SLIM_POD_HEADER

namespace y {
namespace serde3 {

namespace detail {
template<typename T>
struct Deconst {
    using type = remove_cvref_t<T>;
};

template<typename... Args>
struct Deconst<std::tuple<Args...>> {
    using type = std::tuple<remove_cvref_t<Args>...>;
};

template<typename A, typename B>
struct Deconst<std::pair<A, B>> {
    using type = std::pair<remove_cvref_t<A>, remove_cvref_t<B>>;
};
}

template<typename T>
using deconst_t = typename detail::Deconst<remove_cvref_t<T>>::type;


namespace detail {

struct TypeHeader {
    u32 name_hash = 0;
    u32 type_hash = 0;

    constexpr bool has_serde() const {
        return type_hash & 0x01;
    }

    constexpr bool is_range() const {
        return type_hash & 0x02;
    }

    constexpr bool operator==(const TypeHeader& other) const {
        return name_hash == other.name_hash && type_hash == other.type_hash;
    }
};

struct MembersHeader {
    u32 member_hash = 0;
    u32 count = 0;

    constexpr bool operator==(const MembersHeader& other) const {
        return member_hash == other.member_hash && count == other.count;
    }
};

struct TrivialHeader {
    TypeHeader type;

    constexpr bool operator==(const TrivialHeader& other) const {
        return type == other.type;
    }
    constexpr bool operator!=(const TrivialHeader& other) const {
        return !operator==(other);
    }
};

struct ObjectHeader {
    TypeHeader type;
    MembersHeader members;

    constexpr bool operator==(const ObjectHeader& other) const {
        return type == other.type && members == other.members;
    }

    constexpr bool operator!=(const ObjectHeader& other) const {
        return !operator==(other);
    }

    constexpr bool operator==(const TrivialHeader& other) const {
        return type == other.type;
    }

    constexpr bool operator!=(const TrivialHeader& other) const {
        return !operator==(other);
    }
};

static_assert(sizeof(TypeHeader) == sizeof(u64));
static_assert(sizeof(MembersHeader) == sizeof(u64));


constexpr u32 ct_str_hash(std::string_view str) {
    u32 hash = 0xec81fb49;
    for(char c : str) {
        hash_combine(hash, u32(c));
    }
    return hash;
}

template<typename T>
constexpr u32 header_type_hash() {
    using naked = deconst_t<T>;

    if constexpr(is_range_v<T>) {
        static_assert(!has_serde3_v<T>);
        using value_type = typename T::value_type;
        u32 hash = header_type_hash<value_type>();
        hash_combine(hash, u32(0xd3189c20));
        return hash | 0x02;
    }

    u32 hash = u32(ct_type_hash<naked>());
    if constexpr(has_serde3_v<T>) {
        hash |= 0x01;
    } else {
        hash &= ~0x01;
    }
    return hash & ~0x02;
}

template<typename T>
constexpr TypeHeader build_type_header(NamedObject<T> obj) {
    return TypeHeader {
        ct_str_hash(obj.name),
        header_type_hash<T>()
    };
}

template<usize I, typename... Args>
constexpr void hash_members(u32& hash, const std::tuple<NamedObject<Args>...>& objects) {
    unused(hash, objects);
    if constexpr(I < sizeof...(Args)) {
        const TypeHeader tpe = build_type_header(std::get<I>(objects));
        hash_combine(hash, tpe.name_hash);
        hash_combine(hash, tpe.type_hash);

        hash_members<I + 1>(hash, objects);
    }
}

template<typename T>
constexpr MembersHeader build_members_header(NamedObject<T> obj) {
    u32 member_hash = 0xafbbc3d1;
    hash_members<0>(member_hash, members(obj.object));
    return MembersHeader {
        member_hash,
        u32(member_count<T>()),
    };
}

template<typename T>
constexpr auto build_header(NamedObject<T> obj) {
#ifdef Y_SLIM_POD_HEADER
    if constexpr(has_serde3_v<T>) {
        return ObjectHeader {
            build_type_header(obj),
            build_members_header(obj)
        };
    } else {
        return TrivialHeader {
            build_type_header(obj)
        };
    }
#else
    return ObjectHeader {
        build_type_header(obj),
        build_members_header(obj)
    };
#endif
}

}
}
}

#endif // Y_SERDE3_HEADERS_H

