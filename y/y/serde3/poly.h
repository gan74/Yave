/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include <y/utils/name.h>

#include <memory>

namespace y {
namespace serde3 {

class WritableArchive;
class ReadableArchive;

namespace detail {

template<typename T>
constexpr u64 poly_type_hash() {
	using naked = remove_cvref_t<T>;
	u64 hash = 0xe50c9771d834a0bb;
	for(char c : ct_type_name<naked>()) {
		hash_combine(hash, u64(c));
	}
	return hash;
}

template<typename Base>
struct PolyType {
	using create_func = std::unique_ptr<Base> (*)();

	struct Type {
		u64 type_id = 0;
		Type* next = nullptr;
		create_func create = nullptr;
		std::string_view name;
	};

	inline static Type* first = nullptr;

	template<typename Derived>
	static void register_type() {
		static Type type{
			poly_type_hash<Derived>(),
			first,
			[]() -> std::unique_ptr<Base> { return std::make_unique<Derived>(); },
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


#define y_serde3_poly_base(base)																															\
	static y::serde3::detail::PolyType<base> _y_serde3_poly_base;																							\
	virtual y::serde3::Result _y_serde3_poly_serialize(y::serde3::WritableArchive&) const = 0;																\
	virtual y::serde3::Result _y_serde3_poly_deserialize(y::serde3::ReadableArchive&) = 0;

#define y_serde3_poly(type)																																	\
	inline static struct _y_register_t { _y_register_t() { _y_serde3_poly_base.register_type<type>(); } void odr() {} } _y_register;						\
	y::serde3::Result _y_serde3_poly_serialize(y::serde3::WritableArchive& arc) const override { _y_register.odr(); return arc.serialize(*this); }			\
	y::serde3::Result _y_serde3_poly_deserialize(y::serde3::ReadableArchive& arc) override     { _y_register.odr(); return arc.deserialize(*this); }



}
}

#endif // Y_SERDE3_POLY_H
