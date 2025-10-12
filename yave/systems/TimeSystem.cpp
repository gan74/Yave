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

#include "TimeSystem.h"

#include <yave/ecs/SystemManager.h>

namespace yave {

TimeSystem::TimeSystem(float scale) : ecs::System("TimeSystem"), _scale(scale) {
}

void TimeSystem::setup(ecs::SystemScheduler& sched) {
    sched.schedule(ecs::SystemSchedule::TickSequential, "Update time", [this]() {
        _duration = _timer.reset();
    });
}

float TimeSystem::dt() const {
    return std::max(0.0f, float(_duration.to_secs()) * _scale);
}

void TimeSystem::set_time_scale(float scale) {
    _scale = scale;
}

float TimeSystem::time_scale() const {
    return _scale;
}

float TimeSystem::dt(const ecs::EntityWorld& world) {
    if(const TimeSystem* s = world.find_system<TimeSystem>()) {
        return s->dt();
    }
    return 0.0f;
}

}

