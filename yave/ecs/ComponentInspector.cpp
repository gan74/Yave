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

#include "ComponentInspector.h"

namespace yave {
namespace ecs {

ComponentInspector::~ComponentInspector() {
}

void ComponentInspector::inspect(const core::String& name, float& f, FloatRole role) {
    switch(role) {
        case FloatRole::Distance:
        case FloatRole::DistanceKilometers:
        case FloatRole::NormalizedLumFlux:
            inspect(name, f, 0.0, std::numeric_limits<float>::max(), role);
        break;

        case FloatRole::Angle:
            inspect(name, f, 0.0, math::to_rad(360.0f), role);
        break;

        case FloatRole::HalfAngle:
            inspect(name, f, 0.0, math::to_rad(180.0f), role);
        break;

        default:
            inspect(name, f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), role);
        break;
    }

}

}
}

