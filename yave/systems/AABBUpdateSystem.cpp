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

#include "AABBUpdateSystem.h"

#include <yave/components/TransformableComponent.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

AABBUpdateSystem::AABBUpdateSystem() : ecs::System("AABBUpdateSystem") {
}

void AABBUpdateSystem::setup() {
    world().make_mutated<TransformableComponent>(world().component_set<TransformableComponent>().ids());
}

void AABBUpdateSystem::tick() {
    ecs::SparseComponentSet<AABB> aabbs;
    for(const AABBTypeInfo& info : _infos) {
        const core::Span<ecs::EntityId> ids = world().recently_mutated(info.type).ids();
        if(ids.is_empty()) {
            continue;
        }
        y_profile_dyn_zone(fmt_c_str("collecting {} {}", ids.size(), world().component_type_name(info.type)));

        aabbs.set_min_capacity(aabbs.size() + ids.size());
        info.collect_aabbs(world(), ids, aabbs);
    }

    for(auto&& [id, comp] : world().query<ecs::Mutate<TransformableComponent>>(aabbs.ids())) {
        auto&& [tr] = comp;
        tr.set_aabb(aabbs[id]);
    }
}

}

