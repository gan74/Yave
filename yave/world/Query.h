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

template<typename T>
struct Mut {};


namespace detail {
template<typename T>
struct query_traits {
    using raw_type = std::remove_cvref_t<T>;
    using type = const raw_type;
    static constexpr bool exclude = false;
};

template<typename T>
struct query_traits<Not<T>> {
    using parent = query_traits<T>;
    using raw_type = typename parent::raw_type;
    using type = typename parent::type;
    static constexpr bool exclude = true;
};

template<typename T>
struct query_traits<Mut<T>> {
    using parent = query_traits<T>;
    using raw_type = typename parent::raw_type;
    using type = std::remove_const_t<typename parent::type>;
    static constexpr bool exclude = parent::exclude;
};


template<bool Raw, typename T>
static constexpr auto build_query_type() {
    using traits = query_traits<T>;
    if constexpr(traits::exclude) {
        return type_pack<>{};
    } else {
        if constexpr(Raw) {
            return type_pack<typename traits::raw_type>{};
        } else {
            return type_pack<typename traits::type>{};
        }
    }
}

template<bool Raw, typename T, typename... Args>
static constexpr auto build_query_type_pack() {
    constexpr auto lhs = build_query_type<Raw, T>();
    if constexpr(sizeof...(Args)) {
        constexpr auto rhs = build_query_type_pack<Raw, Args...>();
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
    public:
        using raw_types = decltype(detail::build_query_type_pack<true, Args...>());
        using types = decltype(detail::build_query_type_pack<false, Args...>());

        Query() = default;

        Query(QueryResult&& r) : _result(std::move(r)) {
        }

        template<typename T>
        auto get(usize index) const {
            constexpr usize type_index = index_in_result<T>();
            using return_type = typename type_at_index<type_index, types>::type;
            return _result.refs[index * types::size + type_index].to_typed_unchecked<return_type>();
        }

        usize size() const {
            return _result.ids.size();
        }

    private:
        template<typename T>
        static constexpr usize index_in_result() {
            return type_index_in_pack<T>(raw_types{});
        }

        QueryResult _result;
};


}


#endif // YAVE_WORLD_QUERY_H

