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

    using DepGroups = core::Span<DependencyGroup>;

    const usize dep_count = std::accumulate(_schedulers.begin(), _schedulers.end(), 0_uu, [](usize acc, const auto& s) {
        return acc + std::accumulate(s->_schedules.begin(), s->_schedules.end(), 0_uu, [](usize m, const auto& s) { return std::max(m, s.tasks.size()); });
    });

    core::ScratchVector<DependencyGroup> tmp_1(dep_count);
    core::ScratchVector<DependencyGroup> tmp_0(dep_count);
    auto* current = &tmp_0;
    auto* next = &tmp_1;

    for(usize i = 0; i != usize(SystemSchedule::Max); ++i) {
        if(!next->is_empty()) {
            std::swap(current, next);
            next->make_empty();
        }

        const bool wait_for_all = i == SystemSchedule::PostUpdate;

        for(const auto& scheduler : _schedulers) {
            SystemScheduler::Schedule& sched = scheduler->_schedules[i];
            for(usize k = 0; k != sched.tasks.size(); ++k) {

                DepGroups wait = wait_for_all
                    ? DepGroups(*current)
                    : (i ? DepGroups(scheduler->_schedules[i - 1].signals) : DepGroups());


                core::ScratchPad<DependencyGroup> all_deps;
                if(!sched.wait_groups[k].is_empty()) {
                    y_debug_assert(std::all_of(sched.wait_groups[k].begin(), sched.wait_groups[k].end(), [](const auto& g) { return !g.is_empty(); }));
                    all_deps = core::ScratchPad<DependencyGroup>(wait.size() + sched.wait_groups.size());
                    std::copy(wait.begin(), wait.begin(), all_deps.begin());
                    std::copy(sched.wait_groups[k].begin(), sched.wait_groups[k].end(), all_deps.begin() + wait.size());
                    wait = all_deps;
                }


                DependencyGroup& signal = sched.signals[k];
                next->push_back(signal);

                const SystemScheduler::Task& task = sched.tasks[k];
                thread_pool.schedule([&]() {
                    y_profile_dyn_zone(fmt_c_str("{}: {}", scheduler->_system->name(), task.name));
                    task.func();
                }, &signal, wait);
            }
        }
    }

    y_profile_zone("waiting for completion");
    thread_pool.wait_for(DepGroups(*next));

    for(const auto& scheduler : _schedulers) {
        scheduler->_timer.reset();
    }
}

}
}

