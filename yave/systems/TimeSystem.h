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
#ifndef YAVE_SYSTEMS_TIMESYSTEM_H
#define YAVE_SYSTEMS_TIMESYSTEM_H

#include <yave/ecs/EntityWorld.h>

#include <y/core/Chrono.h>

namespace yave {

class TimeSystem : public ecs::System {
    public:
        TimeSystem(float scale = 1.0);

        void setup(ecs::SystemScheduler& sched) override;

        float dt() const;

        void set_time_scale(float scale);
        float time_scale() const;


        static float dt(const ecs::EntityWorld& world);

    private:
        float _scale = 1.0;

        core::Duration _duration;
        core::StopWatch _timer;
};

}

#endif // YAVE_SYSTEMS_TIMESYSTEM_H

