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
#ifndef YAVE_ECS_SYSTEMMANAGER_H
#define YAVE_ECS_SYSTEMMANAGER_H

#include "System.h"
#include "EntityGroup.h"

#include <y/concurrent/JobSystem.h>


namespace yave {
namespace ecs {

enum class SystemSchedule {
    TickSequential,

    Tick,

    Update,

    PostUpdate,

    Max
};

class SystemJobHandle {
    public:
        SystemJobHandle() = default;

        bool is_valid() const {
            return _handle != u32(-1);
        }

    private:
        friend class SystemScheduler;
        friend class SystemManager;

        SystemJobHandle(u32 h) : _handle(h) {
        }

        u32 _handle = u32(-1);
};

class SystemScheduler : NonMovable {
    public:
        struct FirstTime {
            bool value = false;
        };

    public:
        SystemScheduler(System* sys, SystemManager* manager, EntityWorld* world);

        template<typename Fn>
        SystemJobHandle schedule(SystemSchedule sched, core::String name, Fn&& func, SystemJobHandle dep = {}) {
            auto& s = _schedules[usize(sched)];

            const SystemJobHandle handle = create_job_handle();

            s.tasks.emplace_back(std::move(name), handle, dep, [this, func](concurrent::JobSystem& js) {
                std::array<ArgumentResolver, function_traits<Fn>::arg_count> args;
                for(ArgumentResolver& arg : args) {
                    arg = ArgumentResolver(this, js);
                }
                std::apply(func, args);
            });

            return handle;
        }

    private:
        friend class SystemManager;

        class ArgumentResolver {
            public:
                ArgumentResolver() = default;
                ArgumentResolver(SystemScheduler* parent, concurrent::JobSystem& js);

                operator concurrent::JobSystem&() const;
                operator const EntityWorld&() const;
                operator FirstTime() const;

                template<typename... Ts>
                operator EntityGroup<Ts...>() const;

            private:
                SystemScheduler* _parent = nullptr;
                concurrent::JobSystem* _job_system = nullptr;
        };

        struct Task {
            core::String name;
            SystemJobHandle handle;
            SystemJobHandle wait_for;
            std::function<void(concurrent::JobSystem&)> func;
        };

        struct Schedule {
            core::Vector<Task> tasks;
        };

        SystemJobHandle create_job_handle();

        std::array<Schedule, usize(SystemSchedule::Max)> _schedules;

        System* _system = nullptr;
        SystemManager* _manager = nullptr;
        EntityWorld* _world = nullptr;

        TickId _first_tick;
};




class SystemManager : NonMovable {
    public:
        SystemManager(EntityWorld* world);

        void run_schedule_seq(concurrent::JobSystem& job_system) const;
        void run_schedule_mt(concurrent::JobSystem& job_system) const;

        SystemJobHandle create_job_handle();

        void reset() {
            _schedulers.make_empty();
            for(const auto& system : _systems) {
                system->reset();
                setup_system(system.get());
            }
        }

        template<typename S, typename... Args>
        S* add_system(Args&&... args) {
            y_profile();
            y_always_assert(!find_system<S>(), "System already exists");

            auto s = std::make_unique<S>(y_fwd(args)...);
            S* system = s.get();

            _systems.emplace_back(std::move(s));
            system->register_world(_world);
            setup_system(system);

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

        template<typename S>
        S* find_system() {
            for(auto& system : _systems) {
                if(S* s = dynamic_cast<S*>(system.get())) {
                    return s;
                }
            }
            return nullptr;
        }


    private:
        void run_stage_seq(SystemSchedule schedule, concurrent::JobSystem& job_system) const;

        void setup_system(System *system);

        core::Vector<std::unique_ptr<SystemScheduler>> _schedulers;
        core::Vector<std::unique_ptr<System>> _systems;
        u32 _next_handle = 0;

        EntityWorld* _world = nullptr;
};

}
}


#endif // YAVE_ECS_SYSTEMMANAGER_H

