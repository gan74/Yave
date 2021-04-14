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
#ifndef Y_SERDE3_POLY_H
#define Y_SERDE3_POLY_H

#include "serde.h"
#include "result.h"

#include <y/utils/name.h>
#include <y/utils/hash.h>

#include <memory>

namespace y {
namespace serde3 {

class WritableArchive;
class ReadableArchive;

using TypeId = u64;

namespace detail {

template<typename T>
Result serialize_one(WritableArchive& arc, const T& t);
template<typename T>
Result deserialize_one(ReadableArchive& arc, T& t);


template<typename T>
constexpr TypeId poly_type_id() {
    using naked = remove_cvref_t<T>;
    return TypeId(ct_type_hash<naked>());
}

template<typename Base>
struct PolyType {
    using create_func = std::unique_ptr<Base> (*)();

    struct Type {
        TypeId type_id = 0;
        Type* next = nullptr;
        create_func create = nullptr;
        std::string_view name;
    };

    inline static Type* first = nullptr;


    static const Type* find_id(TypeId id) {
        for(auto* t = first; t; t = t->next) {
            if(t->type_id == id) {
                return t;
            }
        }
        return nullptr;
    }

    static const Type* find_name(std::string_view name) {
        for(auto* t = first; t; t = t->next) {
            if(t->name == name) {
                return t;
            }
        }
        return nullptr;
    }

    template<typename Derived>
    static void register_type() {
        static Type type{
            poly_type_id<Derived>(),
            first,
            []() -> std::unique_ptr<Base> {
                // checking for size to trigger error on imcomplete types
                static_assert(sizeof(Derived) > 0);
                if constexpr(std::is_default_constructible_v<Derived>) {
                    return std::make_unique<Derived>();
                }
                y_breakpoint;
                return nullptr;
            },
            ct_type_name<Derived>()
        };
        first = &type;
    }

    static usize registered_type_count() {
        usize i = 0;
        for(auto* t = first; t; t = t->next) {
            ++i;
        }
        return i;
    }
};

}

#define y_serde3_poly_qual(type, virt, over)                                                                        \
    inline static struct _y_register_t {                                                                            \
        _y_register_t() { _y_serde3_poly_base.register_type<type>(); }                                              \
        void used() {}                                                                                              \
    } _y_register;                                                                                                  \
    virt y::serde3::TypeId _y_serde3_poly_type_id() const over {                                                    \
        return y::serde3::detail::poly_type_id<y::remove_cvref_t<decltype(*this)>>();                               \
    }                                                                                                               \
    virt y::serde3::Result _y_serde3_poly_serialize(y::serde3::WritableArchive& arc) const over {                   \
        _y_register.used();                                                                                         \
        return y::serde3::detail::serialize_one(arc, *this);                                                        \
    }                                                                                                               \
    virt y::serde3::Result _y_serde3_poly_deserialize(y::serde3::ReadableArchive& arc) over {                       \
        _y_register.used();                                                                                         \
        return y::serde3::detail::deserialize_one(arc, *this);                                                      \
    }



#define y_serde3_poly_abstract_base(base)                                                                           \
    static y::serde3::detail::PolyType<base> _y_serde3_poly_base;                                                   \
    virtual y::serde3::TypeId _y_serde3_poly_type_id() const = 0;                                                   \
    virtual y::serde3::Result _y_serde3_poly_serialize(y::serde3::WritableArchive&) const = 0;                      \
    virtual y::serde3::Result _y_serde3_poly_deserialize(y::serde3::ReadableArchive&) = 0;

#define y_serde3_poly_base(base)                                                                                    \
    static y::serde3::detail::PolyType<base> _y_serde3_poly_base;                                                   \
    y_serde3_poly_qual(base, virtual, /*...*/)

#define y_serde3_poly(base)                                                                                         \
    y_serde3_poly_qual(base, /*   */, override)


}
}

#endif // Y_SERDE3_POLY_H

