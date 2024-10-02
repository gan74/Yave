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
#ifndef YAVE_ECS_SYSTEMMANAGER_H
#define YAVE_ECS_SYSTEMMANAGER_H

#include "System.h"
#include "EntityGroup.h"

#include <y/concurrent/StaticThreadPool.h>


namespace yave {
namespace ecs {

enum class SystemSchedule {
    // Tick is always first.
    // Ticks for different systems might run in parallel
    Tick,

    // Update happens after tick for a given system.
    // Updates might still run while other systems are running their tick
    Update,

    // Run after all updates are complete
    PostUpdate,

    Max
};

class SystemScheduler : NonMovable {
    public:
        using DependencyGroup = concurrent::DependencyGroup;

        struct FirstTime {
            bool value = false;
        };

        struct ThreadId {
            u32 id = 0;
            u32 total = 1;
        };

    public:
        SystemScheduler(System* sys, EntityWorld* world);

        template<typename Fn>
        DependencyGroup schedule(SystemSchedule sched, core::String name, Fn&& func, core::Span<DependencyGroup> wait = {}) {
            return schedule_internal(sched, std::move(name), y_fwd(func), wait, false);
        }

        template<typename Fn>
        DependencyGroup schedule_mt(SystemSchedule sched, core::String name, Fn&& func, core::Span<DependencyGroup> wait = {}) {
            return schedule_internal(sched, std::move(name), y_fwd(func), wait, true);
        }

    private:
        friend class SystemManager;

        class ArgumentResolver {
            public:
                ArgumentResolver() = default;
                ArgumentResolver(SystemScheduler* parent, ThreadId id);

                operator const EntityWorld&() const;

                operator FirstTime() const;
                operator ThreadId() const;

                template<typename... Ts>
                operator EntityGroup<Ts...>() const;

            private:
                SystemScheduler* _parent = nullptr;
                ThreadId _thread_id;
        };

        struct Task {
            core::String name;
            std::function<void(ThreadId)> func;
            DependencyGroup signal;
            core::Vector<DependencyGroup> wait;
            bool multi_thread = false;
        };

        struct Schedule {
            core::Vector<Task> tasks;
        };


        template<typename Fn>
        DependencyGroup schedule_internal(SystemSchedule sched, core::String name, Fn&& func, core::Span<DependencyGroup> wait, bool multi_thread) {
            auto& schedule = _schedules[usize(sched)];
            Task& task = schedule.tasks.emplace_back();

            task.name = std::move(name);
            task.wait = wait;
            task.multi_thread = multi_thread;

            task.func = [this, func](ThreadId id) {
                std::array<ArgumentResolver, function_traits<Fn>::arg_count> args;
                std::fill(args.begin(), args.end(), ArgumentResolver(this, id));
                std::apply(func, args);
            };

            return task.signal;
        }


        std::array<Schedule, usize(SystemSchedule::Max)> _schedules;

        System* _system = nullptr;
        EntityWorld* _world = nullptr;

        TickId _first_tick;
};




class SystemManager : NonCopyable {
    public:
        using DependencyGroup = concurrent::DependencyGroup;

        SystemManager(EntityWorld* world);

        void run_schedule(concurrent::StaticThreadPool& thread_pool) const;

        template<typename S, typename... Args>
        S* add_system(Args&&... args) {
            y_profile();
            y_always_assert(!find_system<S>(), "System already exists");

            auto s = std::make_unique<S>(y_fwd(args)...);
            S* system = s.get();

            _systems.emplace_back(std::move(s));
            SystemScheduler& sched = *_schedulers.emplace_back(std::make_unique<SystemScheduler>(system, _world));

            {
                y_profile_zone("setup");
                system->register_world(_world);
                system->setup(sched);
            }
            return system;
        }

        template<typename S>
        const S* find_system() const {
            for(auto& system : _systems) {
                if(const S* s = dynamic_cast<const S*>(system.get())) {
                    return s;
                }
            }
            return nullptr;
        }

    private:
        core::Vector<std::unique_ptr<SystemScheduler>> _schedulers;
        core::Vector<std::unique_ptr<System>> _systems;

        EntityWorld* _world = nullptr;
};

}
}


#endif // YAVE_ECS_SYSTEMMANAGER_H

