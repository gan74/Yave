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
#ifndef YAVE_ECS_TRIGGERMANAGER_H
#define YAVE_ECS_TRIGGERMANAGER_H

#include "TriggerQueue.h"

#include <y/core/FixedArray.h>

#include <memory>
#include <concepts>
#include <algorithm>

namespace yave {
namespace ecs {

class TriggerManager : NonMovable {
    public:
        TriggerManager();

        void dispatch(EntityWorld& world);

        bool is_listened(TriggerTypeIndex type, EntityId target) const;

        void unsubscribe(TriggerTypeIndex type, EntityId target, TriggerHandler* handler);



        template<typename T>
        bool is_listened(EntityId target) const {
            return is_listened(trigger_index<T>(), target);
        }

        template<typename T>
        void emit(EntityId target, const T& payload) {
            if(TriggerQueue<T>* queue = find_queue<T>(); queue && queue->is_listened(target)) {
                queue->push(target, payload);
            }
        }

        template<typename T, std::invocable F>
        void emit(EntityId target, F&& make) {
            if(TriggerQueue<T>* queue = find_queue<T>(); queue && queue->is_listened(target)) {
                queue->push(target, make());
            }
        }

        template<typename T>
        void subscribe(EntityId target, TriggerHandler* handler) {
            auto& queue = _queues[usize(trigger_index<T>())];
            if(!queue) {
                queue = std::make_unique<TriggerQueue<T>>();
            }
            queue->subscribe(target, handler);
        }

        template<typename T>
        void unsubscribe(EntityId target, TriggerHandler* handler) {
            unsubscribe(trigger_index<T>(), target, handler);
        }

    private:
        template<typename T>
        TriggerQueue<T>* find_queue() {
            return static_cast<TriggerQueue<T>*>(find_queue(trigger_index<T>()));
        }

        TriggerQueueBase* find_queue(TriggerTypeIndex type);


        core::FixedArray<std::unique_ptr<TriggerQueueBase>> _queues;
};




class TriggerSubscriber : NonMovable {
    public:
        using TypeList = core::SmallVector<TriggerTypeIndex, 4>;

        TriggerSubscriber(TriggerManager& manager, EntityId target, TriggerHandler* handler, TypeList& types) :
                _manager(manager), _target(target), _handler(handler), _types(types) {
        }

        template<typename T>
        void subscribe() {
            const TriggerTypeIndex type = trigger_index<T>();
            if(std::find(_types.begin(), _types.end(), type) == _types.end()) {
                _manager.subscribe<T>(_target, _handler);
                _types << type;
            }
        }

    private:
        TriggerManager& _manager;
        EntityId _target;
        TriggerHandler* _handler = nullptr;
        TypeList& _types;
};

}
}

#endif // YAVE_ECS_TRIGGERMANAGER_H

