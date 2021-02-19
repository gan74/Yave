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
#ifndef YAVE_MESHES_VERTEX_H
#define YAVE_MESHES_VERTEX_H

#include <yave/yave.h>

#include <array>

namespace yave {

struct Vertex {
    math::Vec3 position;
    math::Vec3 normal;
    math::Vec3 tangent;
    math::Vec2 uv;
};

using IndexedTriangle = std::array<u32, 3>;

static_assert(sizeof(IndexedTriangle) == 3 * sizeof(u32));

struct SkinWeights {
    static constexpr usize size = 4;

    math::Vec<size, u32> indexes;
    math::Vec<size, float> weights;
};

struct SkinnedVertex {
    Vertex vertex;
    SkinWeights weights;
};

static_assert(std::is_trivially_copyable_v<SkinnedVertex>, "SkinnedVertex should be trivially copyable");
static_assert(std::is_trivially_copyable_v<IndexedTriangle>, "IndexedTriangle should be trivially copyable");

}

#endif // YAVE_MESHES_VERTEX_H

