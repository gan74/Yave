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
#ifndef YAVE_WORLD_QUERY_H
#define YAVE_WORLD_QUERY_H

#include "ComponentRef.h"
#include "EntityId.h"

#include <y/core/Vector.h>

namespace yave {

template<typename T>
struct Not {};


namespace detail {
template<typename T>
struct query_traits {
    using raw_type = std::remove_cvref_t<T>;
    using type = const raw_type;
    static constexpr bool exclude = false;
};

template<typename T>
struct query_traits<Not<T>> {
    using raw_type = typename query_traits<T>::raw_type;
    using type = typename query_traits<T>::type;
    static constexpr bool exclude = true;
};

template<typename T>
static constexpr auto filter_query_type() {
    if constexpr(query_traits<T>::exclude) {
        return type_pack<>{};
    } else {
        return type_pack<T>{};
    }
}

template<typename T, typename... Args>
static constexpr auto filter_query_types() {
    constexpr auto lhs = filter_query_type<T>();
    if constexpr(sizeof...(Args)) {
        constexpr auto rhs = filter_query_types<Args...>();
        return concatenate_packs(lhs, rhs);
    } else {
        return lhs;
    }
}
}


struct QueryResult : NonCopyable {
    core::Vector<EntityId> ids;
    core::Vector<UncheckedComponentRef> refs;
};



template<typename... Args>
class Query {
    using types = decltype(detail::filter_query_types<Args...>());

    public:
        Query() = default;

        Query(QueryResult&& r) : _result(std::move(r)) {
        }

        template<typename T>
        auto get(usize index) const {
            using traits = detail::query_traits<T>;
            constexpr usize type_index = type_index_in_pack<typename traits::raw_type>(types{});
            return _result.refs[index * types::size + type_index].to_typed_unchecked<typename traits::type>();
        }

        usize size() const {
            return _result.ids.size();
        }

    private:
        QueryResult _result;
};


}


#endif // YAVE_WORLD_QUERY_H

