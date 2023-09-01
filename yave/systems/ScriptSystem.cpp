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

#include "ScriptSystem.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

ScriptSystem::ScriptSystem() : ecs::System("ScriptSystem") {
    _state.open_libraries();

    script::bind_math_types(_state);
    script::bind_ecs_types(_state);

    script::bind_component_type<TransformableComponent>(_state);
    script::bind_component_type<PointLightComponent>(_state);
}

void ScriptSystem::update(float) {
    ScriptWorldComponent* scripts_comp = world().get_or_add_world_component<ScriptWorldComponent>();

    _state["world"] = &world();

    for(auto& one_shot : scripts_comp->_one_shot) {
        if(!one_shot.done) {
            one_shot.done = true;
            try {
                _state.safe_script(one_shot.code);
            } catch(std::exception& e) {
                log_msg(fmt("Lua error in one shot: {}", e.what()), Log::Error);
            }
        }
    }

    for(auto& script : scripts_comp->_scripts) {
        y_profile_dyn_zone(script.name.data());
        try {
            _state.safe_script(script.code);
        } catch(std::exception& e) {
            log_msg(fmt("Lua error in {}: {}", script.name, e.what()), Log::Error);
        }
    }

    for(auto& once : scripts_comp->_once) {
        try {
            _state.safe_script(once.code);
        } catch(std::exception& e) {
            once.result->result = core::Err(core::String(e.what()));
        }
        once.result->done = true;
    }
    scripts_comp->_once.clear();
}

}

