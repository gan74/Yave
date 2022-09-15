/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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
#ifndef YAVE_SCRIPT_SCRIPT_H
#define YAVE_SCRIPT_SCRIPT_H

#include <yave/yave.h>

#include <yave/ecs/EntityWorld.h>

#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/core/HashMap.h>

#include <y/reflect/reflect.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>


namespace yave {
namespace script {

namespace detail {
core::FlatHashMap<core::String, lua_CFunction>& component_set_casts(sol::state_view s);
}

static_assert(sol::is_container_v<core::Vector<int>>);

template<typename T, typename M>
auto to_property(M T::* member) {
    if constexpr(std::is_same_v<M, core::String>) {
        return sol::property(
            [=](T* obj, std::string_view view) { (obj->*member) = view; },
            [=](T* obj) { return (obj->*member).view(); }
        );
    } else {
        return member;
    }
}


void bind_math_types(sol::state_view state);
void bind_ecs_types(sol::state_view state);



template<typename T>
void bind_component_type(sol::state_view state) {
    using LuaComponentSet = ecs::SparseComponentSet<T>;

    const core::String type_name = T::_y_reflect_type_name;
    auto type = state.new_usertype<LuaComponentSet>(type_name + "Set");

    type["__index"] = [](const LuaComponentSet* set, ecs::EntityId id) {
        y_debug_assert(set);
        return set->try_get(id);
    };

    type["__len"] = [](const LuaComponentSet* set) {
        y_debug_assert(set);
        return set->size();
    };

    detail::component_set_casts(state)[type_name] = [](lua_State* l) -> int {
        if(sol::stack::check_usertype<ecs::EntityWorld>(l)) {
            auto& world = sol::stack::get_usertype<ecs::EntityWorld>(l);
            if(auto* typed_set = dynamic_cast<LuaComponentSet*>(&world.component_set<T>())) {
                sol::stack::push<LuaComponentSet*>(l, typed_set);
                return 1;
            }
        }

        sol::stack::push(l, sol::nil);
        return 1;
    };
}

}
}


namespace sol {
template<typename T>
struct is_container<yave::ecs::SparseComponentSet<T>> : std::false_type {};
}


inline int sol_lua_push(sol::types<y::core::String>, lua_State* l, const y::core::String& str) {
    return sol::stack::push(l, str.view());
}

inline y::core::String sol_lua_get(sol::types<y::core::String>, lua_State* l, int index, sol::stack::record& tracking) {
    return y::core::String(sol::stack::get<std::string_view>(l, lua_absindex(l, index)));
}

template <typename Handler>
inline bool sol_lua_check(sol::types<y::core::String>, lua_State* l, int index, Handler&& handler, sol::stack::record& tracking) {
    tracking.use(1);
    return sol::stack::check<std::string_view>(l, lua_absindex(l, index), handler);
}

#endif // YAVE_SCRIPT_SCRIPT_H
