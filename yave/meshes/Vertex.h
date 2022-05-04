/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

struct FullVertex {
    math::Vec3 position;
    math::Vec3 normal;
    math::Vec4 tangent;
    math::Vec2 uv;
};

struct PackedVertex {
    math::Vec3 position;
    u32 packed_normal = 0;
    u32 packed_tangent_sign = 0;
    math::Vec2 uv;
};



inline u32 pack_2_10_10_10(math::Vec3 v, bool sign = false) {
    v = v * 0.5f + 0.5f;
    const float norm = float(0x03FF);
    const u32 packed =
        (u32(v.x() * norm) << 20) |
        (u32(v.y() * norm) << 10) |
        (u32(v.z() * norm) <<  0) |
        (sign ? (1 << 31) : 0);
    return packed;
}

inline math::Vec4 unpack_2_10_10_10(u32 packed) {
    return math::Vec4(
        (math::Vec3(
            (packed >> 20) & 0x03FF,
            (packed >> 10) & 0x03FF,
            (packed >>  0) & 0x03FF
        ) / float(0x03FF)) * 2.0f - 1.0f,
        (packed >> 30 == 0) ? 1.0f : -1.0f
    );
}

inline PackedVertex pack_vertex(const FullVertex& v) {
    return PackedVertex {
        v.position,
        pack_2_10_10_10(v.normal),
        pack_2_10_10_10(v.tangent.to<3>(), v.tangent.w() < 0.0f),
        v.uv
    };
}

inline FullVertex unpack_vertex(const PackedVertex& v) {
    return FullVertex {
        v.position,
        unpack_2_10_10_10(v.packed_normal).to<3>(),
        unpack_2_10_10_10(v.packed_tangent_sign),
        v.uv
    };
}





using IndexedTriangle = std::array<u32, 3>;

static_assert(sizeof(IndexedTriangle) == 3 * sizeof(u32));

struct SkinWeights {
    static constexpr usize size = 4;

    math::Vec<size, u32> indexes;
    math::Vec<size, float> weights;
};

struct SkinnedVertex {
    PackedVertex vertex;
    SkinWeights weights;
};

static_assert(std::is_trivially_copyable_v<SkinnedVertex>, "SkinnedVertex should be trivially copyable");
static_assert(std::is_trivially_copyable_v<IndexedTriangle>, "IndexedTriangle should be trivially copyable");

}

#endif // YAVE_MESHES_VERTEX_H

