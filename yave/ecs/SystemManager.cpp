/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

SystemScheduler::ArgumentResolver::ArgumentResolver(SystemScheduler* parent) : _parent(parent) {
}

SystemScheduler::ArgumentResolver::operator const EntityWorld&() const {
    y_debug_assert(_parent);
    return *_parent->_world;
}

SystemScheduler::ArgumentResolver::operator SystemScheduler::FirstTime() const {
    return FirstTime { _parent->_world->tick_id() == _parent->_first_tick };
}

SystemScheduler::SystemScheduler(System* sys, SystemManager* manager, EntityWorld *world) : _system(sys), _manager(manager), _world(world), _first_tick(_world->tick_id().next()) {
}

SystemJobHandle SystemScheduler::create_job_handle() {
    return _manager->create_job_handle();
}








SystemManager::SystemManager(EntityWorld* world) : _world(world) {
    y_debug_assert(_world);
}

void SystemManager::run_stage_seq(SystemSchedule schedule) const {
    y_profile();

    for(const auto& scheduler : _schedulers) {
        SystemScheduler::Schedule& sched = scheduler->_schedules[usize(schedule)];
        for(usize i = 0; i != sched.tasks.size(); ++i) {
            const auto& task = sched.tasks[i];
            y_profile_dyn_zone(fmt_c_str("{}: {}", scheduler->_system->name(), task.name));
            task.func();
        }
    }
}

void SystemManager::run_schedule_seq() const {
    y_profile();

    for(usize t = 0; t != usize(SystemSchedule::Max); ++t) {
        run_stage_seq(SystemSchedule(t));
    }
}

void SystemManager::run_schedule_mt(concurrent::JobSystem& job_system) const {
    y_profile();

    run_stage_seq(SystemSchedule::TickSequential);

    usize task_count = 0;
    usize max_tasks = 0;
    {
        for(usize i = usize(SystemSchedule::Tick); i != usize(SystemSchedule::Max); ++i) {
            for(const auto& scheduler : _schedulers) {
                SystemScheduler::Schedule& sched = scheduler->_schedules[i];
                task_count += sched.tasks.size();
                max_tasks = std::max(max_tasks, sched.tasks.size());
            }
        }
    }


    auto handles = core::ScratchPad<concurrent::JobSystem::JobHandle>(_next_handle);
    auto prev = core::Vector<concurrent::JobSystem::JobHandle>::with_capacity(max_tasks + 1);
    auto next = core::Vector<concurrent::JobSystem::JobHandle>::with_capacity(max_tasks);

    std::atomic<u32> completed = 0;
    u32 submitted = 0;

    for(usize i = usize(SystemSchedule::Tick); i != usize(SystemSchedule::Max); ++i) {
        for(const auto& scheduler : _schedulers) {
            SystemScheduler::Schedule& sched = scheduler->_schedules[i];
            for(const SystemScheduler::Task& task : sched.tasks) {
                ++submitted;

                if(task.wait_for.is_valid()) {
                    if(const auto h = handles[task.wait_for._handle]; !h.is_empty()) {
                        prev.emplace_back(handles[task.wait_for._handle]);
                    }
                }

                auto job = job_system.schedule([&]() {
                    y_profile_dyn_zone(fmt_c_str("{}: {}", scheduler->_system->name(), task.name));
                    task.func();
                    ++completed;
                }, prev);


                if(task.wait_for.is_valid()) {
                    prev.pop();
                }

                y_debug_assert(handles[task.handle._handle].is_empty());
                handles[task.handle._handle] = job;

                next.emplace_back(std::move(job));
            }
        }
        if(!next.is_empty()) {
            prev.swap(next);
            next.make_empty();
        }
    }

    job_system.wait(prev);

    y_debug_assert(submitted == completed);
}


void SystemManager::setup_system(System* system) {
    SystemScheduler& sched = *_schedulers.emplace_back(std::make_unique<SystemScheduler>(system, this, _world));

    y_profile_zone("setup");
    system->setup(sched);
}

SystemJobHandle SystemManager::create_job_handle() {
    return _next_handle++;
}

}
}

