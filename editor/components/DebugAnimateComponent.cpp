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

#include "DebugAnimateComponent.h"

#include <yave/ecs/ComponentInspector.h>

#include <y/math/random.h>
#include <random>

namespace editor {

DebugAnimateComponent::DebugAnimateComponent() {
    const core::Duration d = core::Chrono::program();

    math::FastRandom rng(d.to_nanos());
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);

    const float phi = distrib(rng) * math::pi<float> * 0.5f;
    const float theta = distrib(rng) * math::pi<float> * 2.0f;
    _axis = math::Vec3(
        std::sin(phi) * std::cos(theta),
        std::sin(phi) * std::sin(theta),
        std::cos(phi)
    );
}

const math::Vec3& DebugAnimateComponent::axis() const {
    return _axis;
}

float DebugAnimateComponent::speed() const {
    return _speed;
}

void DebugAnimateComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Speed", _speed);
}

}

