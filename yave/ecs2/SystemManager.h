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
#ifndef YAVE_ECS2_SYSTEMMANAGER_H
#define YAVE_ECS2_SYSTEMMANAGER_H

#include "System.h"
#include "EntityGroup.h"

#include <y/concurrent/StaticThreadPool.h>


namespace yave {
namespace ecs2 {

enum SystemSchedule {
    Tick = 0,
    Update,
    PostUpdate,

    Max
};

class SystemScheduler : NonMovable {
    class ArgumentMaker {
        public:
            ArgumentMaker() = default;

            ArgumentMaker(EntityWorld* world) : _world(world) {
            }

            operator const EntityWorld&() const {
                y_debug_assert(_world);
                return *_world;
            }

            template<typename... Ts>
            operator const EntityGroup<Ts...>&() const;

        private:
            EntityWorld* _world = nullptr;
    };

    public:
        SystemScheduler(System* sys, EntityWorld* world) : _system(sys), _world(world) {
        }

        template<typename Fn>
        void schedule(SystemSchedule sched, Fn&& func) {
            _schedule[usize(sched)].emplace_back([this, func]() {
                std::array<ArgumentMaker, function_traits<Fn>::arg_count> args;
                std::fill(args.begin(), args.end(), _world);
                std::apply(func, args);
            });
        }

    private:
        friend class SystemManager;

        std::array<core::SmallVector<std::function<void()>, 4>, usize(SystemSchedule::Max)> _schedule;

        System* _system = nullptr;
        EntityWorld* _world = nullptr;
};




class SystemManager : NonCopyable {
    public:
        SystemManager(EntityWorld* world);

        void run_schedule(concurrent::StaticThreadPool& thread_pool) const;

        template<typename S, typename... Args>
        S* add_system(Args&&... args) {
            y_always_assert(!find_system<S>(), "System already exists");

            auto s = std::make_unique<S>(y_fwd(args)...);
            S* system = s.get();

            _systems.emplace_back(std::move(s));
            SystemScheduler& sched = *_schedulers.emplace_back(std::make_unique<SystemScheduler>(system, _world));

            system->_world = _world;
            system->setup(sched);
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


#endif // YAVE_ECS2_SYSTEMMANAGER_H

