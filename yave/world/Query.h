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

namespace yave {
namespace detail {
template<typename T, typename U, typename... Args>
static constexpr usize index_of_type() {
    if constexpr(std::is_same_v<T, std::remove_cvref_t<U>>) {
        return 0;
    } else {
        return index_of_type<T, Args...>() + 1;
    }
}
}

struct QueryResult : NonCopyable {
    core::Vector<EntityId> ids;
    core::Vector<UntypedComponentRef> refs;
};



template<typename... Args>
class Query {
    public:
        Query() = default;

        Query(QueryResult&& r) : _result(std::move(r)) {
        }

        template<typename T>
        ComponentRef<T> get(usize index) const {
            constexpr usize type_index = detail::index_of_type<T, Args...>();
            return _result.refs[index * sizeof...(Args) + type_index].to_typed_unchecked<T>();
        }

        usize size() const {
            return _result.ids.size();
        }

    private:
        QueryResult _result;
};


}


#endif // YAVE_WORLD_QUERY_H

