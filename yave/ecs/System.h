/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef YAVE_ECS_SYSTEM_H
#define YAVE_ECS_SYSTEM_H

#include <yave/ecs/ecs.h>

#include <y/core/String.h>

namespace yave {
namespace ecs {

class System : NonCopyable {
    public:
        System(core::String name, float fixed_update_time = 0.0f) : _name(std::move(name)), _fixed_update_time(fixed_update_time) {
        }

        virtual ~System() = default;

        const core::String& name() const {
            return _name;
        }

        virtual void tick(EntityWorld&) {
            // nothing
        }

        virtual void update(EntityWorld&, float dt) {
            unused(dt);
            // nothing
        }

        virtual void fixed_update(EntityWorld&, float dt) {
            unused(dt);
            // nothing
        }

        virtual void setup(EntityWorld&) {
            // nothing
        }

        virtual void destroy(EntityWorld&) {
            // nothing
        }

        virtual void reset(EntityWorld& world) {
            destroy(world);
            setup(world);
        }

        float fixed_update_time() const {
            return _fixed_update_time;
        }

        void schedule_fixed_update(EntityWorld& world, float dt) {
            if(_fixed_update_time <= 0.0f) {
                if(_fixed_update_time > -1.0f) {
                    fixed_update(world, dt);
                }
                return;
            }

            _fixed_update_acc += dt;
            while(_fixed_update_acc > _fixed_update_time) {
                fixed_update(world, _fixed_update_time);
                _fixed_update_acc -= _fixed_update_time;
            }
        }

    private:
        core::String _name;
        float _fixed_update_time = 0.0f;
        float _fixed_update_acc = 0.0f;
};

}
}

#endif // YAVE_ECS_SYSTEM_H

