/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
        System(core::String name) : _name(std::move(name)) {
        }

        virtual ~System() = default;

        const core::String& name() const {
            return _name;
        }


        virtual void tick(EntityWorld& world) = 0;

        virtual void setup(EntityWorld&) {
            // nothing
        }

        virtual void reset(EntityWorld& world) {
            setup(world);
        }

    private:
        core::String _name;
};

template<typename F>
class FunctorSystem : public System {
    public:
        FunctorSystem(F&& f) : System("FunctorSystem"), _func(std::move(f)) {
        }

        void tick(EntityWorld& world) override {
            _func(world);
        }

    private:
        F _func;
};

}
}

#endif // YAVE_ECS_SYSTEM_H
