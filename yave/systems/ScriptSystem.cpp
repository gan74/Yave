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
#include <yave/components/ScriptWorldComponent.h>


#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

ScriptSystem::ScriptSystem() : ecs::System("ScriptSystem") {
    _state.open_libraries();

    {
        auto type = _state.new_usertype<ecs::EntityWorld>("World");

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

    {
        auto type = _state.new_usertype<ecs::EntityId>("EntityId");

        type["__tostring"] = [](ecs::EntityId id) { return fmt("[%; %]", id.index(), id.version()); };
    }

    {
        using Transform = math::Transform<>;
        auto type = _state.new_usertype<Transform>("Transform");
        type["position"] = [](const Transform& tr) { return tr.position(); };
    }

    script::bind_type<TransformableComponent>(_state);
}

void ScriptSystem::update(ecs::EntityWorld& world, float dt) {
    const ScriptWorldComponent* scripts_comp = world.get_or_add_world_component<ScriptWorldComponent>();

    _state["world"] = &world;

    for(const core::String& script : scripts_comp->scripts()) {
        try {
            _state.safe_script(script);
        } catch(std::exception& e) {
            log_msg(fmt("Lua error: %", e.what()), Log::Error);
        }
    }
}

}

