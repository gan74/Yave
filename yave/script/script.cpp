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

#include "script.h"

#include <y/utils/format.h>
#include <y/utils/log.h>

extern "C" {
Y_DLL_EXPORT void yave_ffi_example() {
    y::log_msg("ffi example", y::Log::Debug);
}

Y_DLL_EXPORT void* yave_ffi_get_ptr(void* ptr) {
    return *static_cast<void**>(ptr);
}
}


namespace yave {
namespace script {

namespace detail {
core::FlatHashMap<core::String, lua_CFunction>& component_set_casts(sol::state_view s) {
    return s.registry()["component_set_casts"];
}
}

void bind_math_types(sol::state_view state) {
    state.script("ffi = require('ffi')");

    state.script(R"#(
        ffi.cdef[[
            typedef struct { float x, y, z; } vec3_t;
            typedef struct { float colums[4][4]; } transform_t;

            void yave_ffi_example();
            void* yave_ffi_get_ptr(void*);
        ]]

        yave = ffi.C

        Vec3 = ffi.metatype('vec3_t', {
            __add = function(a, b) return Vec3(a.x + b.x, a.y + b.y, a.z + b.z) end,
            __len = function(a) return math.sqrt(a.x * a.x + a.y * a.y + a.z * a.z) end,
            __tostring = function(a) return '[' .. tostring(a.x) .. ', ' .. tostring(a.y) .. ', ' .. tostring(a.z) .. ']' end,
            __index = {
                from_yave = function(a)
                    local v = Vec3()
                    ffi.copy(v, yave.yave_ffi_get_ptr(a), ffi.sizeof('vec3_t'))
                    return v
                end,
            },
        })

        Transform = ffi.metatype('transform_t', {
            __index = {
                identity = function()
                    local t = Transform()
                    t.colums[0][0] = 1.0
                    t.colums[1][1] = 1.0
                    t.colums[2][2] = 1.0
                    t.colums[3][3] = 1.0
                    return t;
                end,

                position = function(t)
                    return Vec3(t.colums[3][0], t.colums[3][1], t.colums[3][2])
                end,

                set_position = function(t, p)
                    t.colums[3][0] = p.x
                    t.colums[3][1] = p.y
                    t.colums[3][2] = p.z
                end,

                from_yave = function(a)
                    local t = Transform()
                    ffi.copy(v, yave.yave_ffi_get_ptr(a), ffi.sizeof('transform_t'))
                    return v
                end,
            },
        })
    )#");
}

void bind_ecs_types(sol::state_view state) {
    state.registry()["component_set_casts"] = std::make_unique<core::FlatHashMap<core::String, lua_CFunction>>();

    {
        auto type = state.new_usertype<ecs::EntityId>("EntityId");

        type["__tostring"] = [](ecs::EntityId id) { return fmt("[%; %]", id.index(), id.version()); };
    }

    {
        auto type = state.new_usertype<ecs::EntityWorld>("World");

        type["set"] = [](lua_State* l) -> int {
            if(sol::stack::check<std::string_view>(l)) {
                const core::String type_name = sol::stack::pop<std::string_view>(l);
                if(lua_CFunction cast_func = detail::component_set_casts(l)[type_name.view()]) {
                    return cast_func(l);
                }
            }
            sol::stack::push(l, sol::nil);
            return 1;
        };

        type["query"] = [](const ecs::EntityWorld& world, sol::variadic_args va) -> core::Vector<ecs::EntityId> {
            core::ScratchVector<core::String> tags(va.size());
            for(auto v : va) {
                tags.emplace_back(v.as<std::string_view>());
            }
            return world.query<>(tags).ids();
        };

        type["component_type_names"] = [](const ecs::EntityWorld& world) -> core::Vector<std::string_view> {
            core::Vector<std::string_view> type_names;
            for(ecs::ComponentTypeIndex t : world.component_types()) {
                type_names.emplace_back(world.component_type_name(t));
            }
            return type_names;
        };
    }
}

}
}


