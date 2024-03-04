/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "SystemManager.h"


#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {
namespace ecs2 {

SystemManager::SystemManager(EntityWorld* world) : _world(world) {
    y_debug_assert(_world);
}

void SystemManager::run_schedule(concurrent::StaticThreadPool& thread_pool) const {
    y_profile();

    constexpr usize deps_count = usize(SystemSchedule::Max) + 1;

    std::array<concurrent::DependencyGroup, deps_count> deps;
    for(usize i = 0; i != usize(SystemSchedule::Max); ++i) {
        for(const auto& sched : _schedulers) {
            for(auto func : sched->_schedule[i]) {
                const core::Span<concurrent::DependencyGroup> wait(deps.data(), i);
                thread_pool.schedule(std::move(func), &deps[i], wait);
            }
        }
    }

    y_profile_zone("waiting for completion");
    thread_pool.wait_for(deps);
}

}
}

