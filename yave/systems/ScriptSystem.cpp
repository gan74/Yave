/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "ScriptSystem.h"

#include <yave/ecs/EntityWorld.h>
#include <yave/components/TransformableComponent.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

template<typename T>
struct LuaComponentSet {
    ecs::SparseComponentSet<T>* ptr = nullptr;
};

static core::FlatHashMap<core::String, lua_CFunction>& cast_functions(sol::state_view s) {
    return s.registry()["component_set_casts"];
}

template<typename T>
static void register_component_type(sol::state& state) {
    script::bind_type<T>(state);

    const core::String type_name = T::_y_reflect_type_name;
    auto type = state.new_usertype<LuaComponentSet<T>>(type_name + "Set");

    type["__index"] = [](const LuaComponentSet<T>& set, ecs::EntityId id) {
        y_debug_assert(set.ptr);
        return set.ptr->try_get(id);
    };
    type["__len"] = [](const LuaComponentSet<T>& set) {
        y_debug_assert(set.ptr);
        return set.ptr->size();
    };

    cast_functions(state)[type_name] = [](lua_State* l) -> int {
        if(sol::stack::check_usertype<ecs::EntityWorld>(l)) {
            auto& world = sol::stack::get_usertype<ecs::EntityWorld>(l);
            if(auto* typed_set = dynamic_cast<ecs::SparseComponentSet<T>*>(&world.component_set<T>())) {
                sol::stack::push<LuaComponentSet<T>>(l, LuaComponentSet<T>{typed_set});
                return 1;
            }
        }

        sol::stack::push(l, sol::nil);
        return 1;
    };
}

static void register_math_types(sol::state& state) {
    {
        using Transform = math::Transform<>;
        auto type = state.new_usertype<Transform>("Transform");
        type["position"] = [](const Transform& tr) { return tr.position(); };
    }

    {
        auto type = state.new_usertype<math::Vec3>("Vec3");
        type["__tostring"] = [](math::Vec3 v) { return fmt("%", v); };
    }
}

static void register_user_types(sol::state& state) {
    register_math_types(state);

    {
        auto type = state.new_usertype<ecs::EntityId>("EntityId");

        type["__tostring"] = [](ecs::EntityId id) { return fmt("[%; %]", id.index(), id.version()); };
    }

    {
        auto type = state.new_usertype<ecs::EntityWorld>("World");

        type["set"] = [](lua_State* l) -> int {
            if(!sol::stack::check<std::string_view>(l)) {
                sol::stack::push(l, sol::nil);
                return 0;
            }
            const core::String type_name = sol::stack::pop<std::string_view>(l);
            lua_CFunction cast_func = cast_functions(l)[type_name.view()];
            y_debug_assert(cast_func);
            return cast_func(l);
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

    state.registry()["component_set_casts"] = std::make_unique<core::FlatHashMap<core::String, lua_CFunction>>();
    register_component_type<TransformableComponent>(state);
}






ScriptSystem::ScriptSystem() : ecs::System("ScriptSystem") {
    _state.open_libraries();

    register_user_types(_state);
}

void ScriptSystem::update(ecs::EntityWorld& world, float dt) {
    ScriptWorldComponent* scripts_comp = world.get_or_add_world_component<ScriptWorldComponent>();

    _state["world"] = &world;

    for(auto& script : scripts_comp->scripts()) {
        y_profile_dyn_zone(script.name.data());
        try {
            if(auto compiled = script.compiled.lock()) {
                compiled->run();
                continue;
            }

            log_msg(fmt("Compiling %", script.name));
            auto& compiled = _compiled.emplace_back(std::make_shared<CompiledScript>());
            compiled->compiled = _state.load(script.code);
            script.compiled = compiled;

            if(!compiled->compiled.valid()) {
                log_msg(fmt("Unable to compile %: %", script.name, std::string_view(sol::to_string(compiled->compiled.status()))), Log::Error);
            } else {
                compiled->run();
            }

        } catch(std::exception& e) {
            log_msg(fmt("Lua error in %: %", script.name, e.what()), Log::Error);
        }
    }
}

}

