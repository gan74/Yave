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
#include "EntityWorld.h"

#include <y/core/ScratchPad.h>

#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {
namespace ecs {

SystemScheduler::ArgumentResolver::ArgumentResolver(SystemScheduler* parent, ThreadId id) : _parent(parent), _thread_id(id) {
}

SystemScheduler::ArgumentResolver::operator const EntityWorld&() const {
    y_debug_assert(_parent);
    return *_parent->_world;
}

SystemScheduler::ArgumentResolver::operator SystemScheduler::FirstTime() const {
    return FirstTime { _parent->_world->tick_id() == _parent->_first_tick };
}

SystemScheduler::ArgumentResolver::operator SystemScheduler::ThreadId() const {
    return _thread_id;
}




SystemScheduler::SystemScheduler(System* sys, EntityWorld* world) : _system(sys), _world(world), _first_tick(_world->tick_id().next()) {
}


SystemManager::SystemManager(EntityWorld* world) : _world(world) {
    y_debug_assert(_world);
}

void SystemManager::run_schedule(concurrent::StaticThreadPool& thread_pool) const {
    y_profile();

    using DepGroups = core::Span<DependencyGroup>;

    const usize dep_count = std::accumulate(_schedulers.begin(), _schedulers.end(), 0_uu, [](usize acc, const auto& s) {
        return acc + std::accumulate(s->_schedules.begin(), s->_schedules.end(), 0_uu, [](usize m, const auto& s) { return std::max(m, s.tasks.size()); });
    }) + 1;


    std::atomic<u32> completed = 0;
    [[maybe_unused]] u32 submitted = 0;

    core::ScratchVector<DependencyGroup> stage_deps(dep_count);
    DependencyGroup previous_stage;

    for(usize k = 0; k != usize(SystemSchedule::Max); ++k) {
        for(const auto& scheduler : _schedulers) {
            auto& schedule = scheduler->_schedules[k];
            for(auto& task : schedule.tasks) {
                core::ScratchVector<DependencyGroup> wait(task.wait.size() + 1);
                std::copy(task.wait.begin(), task.wait.end(), std::back_inserter(wait));
                if(!previous_stage.is_empty()) {
                    wait.push_back(previous_stage);
                }

                task.signal.reset();

                const u32 thread_count = task.multi_thread ? 8 : 1;
                for(u32 i = 0; i != thread_count; ++i) {
                    const SystemScheduler::ThreadId thread_id = {i, thread_count};
                    thread_pool.schedule([&, thread_id] {
                        y_profile_dyn_zone(fmt_c_str("{}: {}", scheduler->_system->name(), task.name));
                        task.func(thread_id);
                        ++completed;
                    }, &task.signal, wait);
                    ++submitted;
                }

                stage_deps.push_back(task.signal);
            }
        }

        if(!previous_stage.is_empty()) {
            stage_deps.push_back(previous_stage);
        }

        DependencyGroup next;
        thread_pool.schedule([&]() {
            y_profile_msg("Stage sync");
        }, &next, stage_deps);

        stage_deps.make_empty();
        previous_stage = next;
    }

    y_profile_zone("waiting for completion");
    thread_pool.process_until_complete(previous_stage);

    y_debug_assert(completed == submitted);
}

}
}

