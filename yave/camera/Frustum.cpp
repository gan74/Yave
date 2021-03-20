/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "Frustum.h"

namespace yave {

Frustum::Frustum(const std::array<math::Vec3, 4>& normals, const math::Vec3& pos, const math::Vec3& forward) :
        _normals(normals),
        _pos(pos),
        _forward(forward) {
}

bool Frustum::is_inside(const math::Vec3& pos, float radius) const {
    const math::Vec3 v = (pos - _pos);
    const float z = v.dot(_forward);
    return z + radius > 0.0f
        && v.dot(_normals[0]) + radius > 0.0f
        && v.dot(_normals[1]) + radius > 0.0f
        && v.dot(_normals[2]) + radius > 0.0f
        && v.dot(_normals[3]) + radius > 0.0f;
}

}

