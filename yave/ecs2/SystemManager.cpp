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

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {
namespace ecs2 {

SystemManager::SystemManager(EntityWorld* world) : _world(world) {
    y_debug_assert(_world);
}

void SystemManager::run_schedule(concurrent::StaticThreadPool& thread_pool) const {
    y_profile();

    using DepGroups = core::Span<concurrent::DependencyGroup>;

    const usize dep_count = std::accumulate(_schedulers.begin(), _schedulers.end(), 0_uu, [](usize acc, const auto& s) {
        return acc + std::accumulate(s->_tasks.begin(), s->_tasks.end(), 0_uu, [](usize m, const auto& f) { return std::max(m, f.size()); });
    });

    std::array<core::ScratchVector<concurrent::DependencyGroup>, 2> deps{dep_count, dep_count};
    auto* current = &deps[0];
    auto* next = &deps[1];

    for(usize i = 0; i != usize(SystemSchedule::Max); ++i) {
        if(!next->is_empty()) {
            std::swap(current, next);
            next->make_empty();
        }

        const bool wait_for_all = i == SystemSchedule::PostUpdate;

        for(const auto& sched : _schedulers) {
            for(const auto& task : sched->_tasks[i]) {
                const DepGroups wait = wait_for_all
                    ? DepGroups(*current)
                    : (i ? DepGroups(sched->_dep_groups[i - 1]) : DepGroups());

                concurrent::DependencyGroup* signal = &sched->_dep_groups[i];
                next->push_back(*signal);

                thread_pool.schedule([&]() {
                    y_profile_dyn_zone(fmt_c_str("{}: {}", sched->_system->name(), task.name));
                    task.func();
                }, signal, wait);
            }
        }
    }

    y_profile_zone("waiting for completion");
    thread_pool.wait_for(DepGroups(*next));
}

}
}

