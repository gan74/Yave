/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#ifdef Y_DEBUG
namespace detail {
thread_local const core::Vector<ComponentAccess>* declared_accesses = nullptr;
}
#endif



void SystemManager::run_task(const SystemScheduler::Task& task) {
#ifdef Y_DEBUG
    const core::Vector<ComponentAccess>* prev = detail::declared_accesses;
    detail::declared_accesses = task.accesses.is_empty() ? nullptr : &task.accesses;
    task.func();
    detail::declared_accesses = prev;
#else
    task.func();
#endif
}



SystemScheduler::SystemScheduler(System* sys, SystemManager* manager, EntityWorld *world) : _system(sys), _manager(manager), _world(world), _first_tick(_world->tick_id().next()) {
}

bool SystemScheduler::is_first_tick() const {
    return _world->tick_id() == _first_tick;
}

SystemJobHandle SystemScheduler::create_job_handle() {
    return _manager->create_job_handle();
}

void SystemScheduler::normalize_accesses(core::Vector<ComponentAccess>& accesses) {
    std::sort(accesses.begin(), accesses.end(), [](const ComponentAccess& a, const ComponentAccess& b) { return a.type < b.type; });

    usize count = 0;
    for(const ComponentAccess& access : accesses) {
        if(count && accesses[count - 1].type == access.type) {
            accesses[count - 1].write |= access.write;
        } else {
            accesses[count++] = access;
        }
    }
    accesses.shrink_to(count);
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
            run_task(task);
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

    using JobHandle = concurrent::JobSystem::JobHandle;

    auto handles = core::ScratchPad<JobHandle>(_next_handle);
    auto jobs = core::ScratchPad<JobHandle>(_task_graph.size());

    core::Vector<JobHandle> prev;
    core::Vector<JobHandle> current;
    core::Vector<JobHandle> deps;

    SystemSchedule current_schedule = SystemSchedule::Max;

    std::atomic<u32> completed = 0;

    for(usize i = 0; i != _task_graph.size(); ++i) {
        const TaskNode& node = _task_graph[i];
        const SystemScheduler::Task& task = *node.task;

        if(node.schedule != current_schedule) {
            if(!current.is_empty()) {
                prev.swap(current);
                current.make_empty();
            }
            current_schedule = node.schedule;
        }

        deps.make_empty();
        deps.push_back(prev.begin(), prev.end());

        if(task.wait_for.is_valid()) {
            if(const JobHandle& h = handles[task.wait_for._handle]; !h.is_empty()) {
                deps << h;
            }
        }

        for(const u32 conflict : node.conflicts) {
            deps << jobs[conflict];
        }

        JobHandle job = job_system.schedule([&node, &completed]() {
            y_profile_dyn_zone(fmt_c_str("{}: {}", node.scheduler->_system->name(), node.task->name));
            run_task(*node.task);
            ++completed;
        }, deps);

        y_debug_assert(handles[task.handle._handle].is_empty());
        handles[task.handle._handle] = job;
        jobs[i] = job;

        current.emplace_back(std::move(job));
    }

    job_system.wait(current);

    y_debug_assert(completed == _task_graph.size());
}

void SystemManager::build_task_graph() {
    y_profile();

    _task_graph.make_empty();

    struct LastAccess {
        u32 writer = u32(-1);
        core::Vector<u32> readers;
    };

    core::Vector<LastAccess> last_accesses;

    for(usize i = usize(SystemSchedule::Tick); i != usize(SystemSchedule::Max); ++i) {
        last_accesses.make_empty();

        for(const auto& scheduler : _schedulers) {
            for(const SystemScheduler::Task& task : scheduler->_schedules[i].tasks) {
                const u32 index = u32(_task_graph.size());

                core::Vector<u32> conflicts;
                for(const ComponentAccess& access : task.accesses) {
                    last_accesses.set_min_size(usize(access.type) + 1);
                    LastAccess& last = last_accesses[usize(access.type)];
                    if(last.writer != u32(-1)) {
                        conflicts << last.writer;
                    }

                    if(access.write) {
                        conflicts.push_back(last.readers.begin(), last.readers.end());
                        last.readers.make_empty();
                        last.writer = index;
                    } else {
                        last.readers << index;
                    }
                }

                std::sort(conflicts.begin(), conflicts.end());
                conflicts.shrink_to(std::unique(conflicts.begin(), conflicts.end()) - conflicts.begin());

                _task_graph.emplace_back(scheduler.get(), &task, SystemSchedule(i), std::move(conflicts));
            }
        }
    }
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

