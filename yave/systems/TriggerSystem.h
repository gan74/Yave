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
#ifndef YAVE_SYSTEMS_TRIGGERSYSTEM_H
#define YAVE_SYSTEMS_TRIGGERSYSTEM_H

#include <yave/ecs/EntityWorld.h>

#include <y/utils/format.h>

namespace yave {

template<typename T>
concept TriggerListener = requires(const T& comp, ecs::TriggerSubscriber& subscriber, ecs::EntityWorld& world, ecs::EntityId id, ecs::TriggerTypeIndex type, const void* payload) {
    comp.subscribe_triggers(subscriber);
    comp.on_trigger(world, id, type, payload);
};


class TriggerSystem : public ecs::System {
    class ComponentSubscriptionsBase : public ecs::TriggerHandler {
        public:
            virtual void setup(ecs::SystemScheduler& sched) = 0;
            virtual void apply(ecs::EntityWorld& world) = 0;
            virtual void collect_all(const ecs::EntityWorld& world) = 0;

            void unsubscribe_all(ecs::TriggerManager& triggers);

        protected:
            void unsubscribe(ecs::TriggerManager& triggers, ecs::EntityId id);

            ecs::SparseIdSet _dirty;
            ecs::SparseComponentSet<ecs::TriggerSubscriber::TypeList> _subscribed;
    };

    template<typename T>
    class ComponentSubscriptions final : public ComponentSubscriptionsBase {
        public:
            void on_trigger(ecs::EntityWorld& world, ecs::EntityId target, ecs::TriggerTypeIndex type, const void* payload) override {
                if(const T* comp = world.component<T>(target)) {
                    comp->on_trigger(world, target, type, payload);
                }
            }

            void setup(ecs::SystemScheduler& sched) override {
                sched.schedule(ecs::SystemSchedule::PostUpdate, fmt("Collect {} trigger subscriptions", ct_type_name<T>()), [this](
                        ecs::EntityGroup<ecs::Changed<T>>&& changed,
                        ecs::EntityGroup<ecs::Deleted<T>>&& deleted) {

                    for(const ecs::EntityId id : changed.ids()) {
                        _dirty.insert(id);
                    }
                    for(const ecs::EntityId id : deleted.ids()) {
                        _dirty.insert(id);
                    }
                });
            }

            void collect_all(const ecs::EntityWorld& world) override {
                for(const ecs::EntityId id : world.component_set<T>().ids()) {
                    _dirty.insert(id);
                }
            }

            void apply(ecs::EntityWorld& world) override {
                ecs::TriggerManager& triggers = world.triggers();
                for(const ecs::EntityId id : _dirty) {
                    unsubscribe(triggers, id);

                    if(const T* comp = world.component<T>(id)) {
                        ecs::TriggerSubscriber::TypeList& types = _subscribed.insert(id);
                        ecs::TriggerSubscriber subscriber(triggers, id, this, types);
                        comp->subscribe_triggers(subscriber);
                        if(types.is_empty()) {
                            _subscribed.erase(id);
                        }
                    }
                }
                _dirty.make_empty();
            }
    };

    public:
        TriggerSystem();

        void setup(ecs::SystemScheduler& sched) override;
        void reset() override;

        template<typename T>
        void register_component_type() {
            static_assert(TriggerListener<T>, "Components registered with TriggerSystem must implement subscribe_triggers and on_trigger");
            _components << std::make_unique<ComponentSubscriptions<T>>();
        }

    private:
        core::Vector<std::unique_ptr<ComponentSubscriptionsBase>> _components;
};

}

#endif // YAVE_SYSTEMS_TRIGGERSYSTEM_H

