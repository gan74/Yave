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

#include "transforms.h"

#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>
#include <yave/graphics/images/ImageData.h>

#include <y/utils/log.h>


#if defined(Y_MSVC) || defined(__SSE4_2__)
#define USE_SIMD
#include <immintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#endif


namespace editor {
namespace import {

template<typename T>
static core::Vector<T> copy(core::Span<T> t) {
    return core::Vector<T>(t);
}

static math::Vec3 h_transform(const math::Vec3& v, const math::Transform<>& tr) {
    const auto h = tr * math::Vec4(v, 1.0f);
    return h.to<3>() / h.w();
}

static math::Vec3 transform(const math::Vec3& v, const math::Transform<>& tr) {
    return tr.to<3, 3>() * v;
}

static FullVertex transform(const FullVertex& v, const math::Transform<>& tr) {
    return FullVertex {
        h_transform(v.position, tr),
        transform(v.normal, tr).normalized(),
        math::Vec4(transform(v.tangent.to<3>(), tr).normalized(), v.tangent.w()),
        v.uv
    };
}

[[maybe_unused]] static PackedVertex transform(const PackedVertex& v, const math::Transform<>& tr) {
    return pack_vertex(transform(unpack_vertex(v), tr));

}

[[maybe_unused]] static BoneTransform transform(const BoneTransform& bone, const math::Transform<>& tr) {
    auto [pos, rot, scale] = tr.decompose();
    return BoneTransform {
            bone.position + pos,
            bone.scale * scale,
            bone.rotation * rot
        };
}


MeshData transform(const MeshData& mesh, const math::Transform<>& tr) {
    y_profile();
    auto vertices = core::vector_with_capacity<PackedVertex>(mesh.vertices().size());
    std::transform(mesh.vertices().begin(), mesh.vertices().end(), std::back_inserter(vertices), [=](const auto& vert) { return transform(vert, tr); });

    if(mesh.has_skeleton()) {
        auto bones = core::vector_with_capacity<Bone>(mesh.bones().size());
        std::transform(mesh.bones().begin(), mesh.bones().end(), std::back_inserter(bones), [=](const auto& bone) {
                return bone.has_parent() ? bone : Bone{bone.name, bone.parent, transform(bone.local_transform, tr)};
            });

        return MeshData(std::move(vertices), copy(mesh.triangles()), copy(mesh.skin()), std::move(bones));
    }

    return MeshData(std::move(vertices), copy(mesh.triangles()), copy(mesh.skin()), copy(mesh.bones()));
}

MeshData compute_tangents(const MeshData& mesh) {
    y_profile();

    auto vertices = copy(mesh.vertices());

    core::FixedArray<math::Vec3> tangents(vertices.size());
    for(IndexedTriangle tri : mesh.triangles()) {
        const math::Vec3 edges[] = {vertices[tri[1]].position - vertices[tri[0]].position, vertices[tri[2]].position - vertices[tri[0]].position};
        const math::Vec2 uvs[] = {vertices[tri[0]].uv, vertices[tri[1]].uv, vertices[tri[2]].uv};
        const float dt[] = {uvs[1].y() - uvs[0].y(), uvs[2].y() - uvs[0].y()};
        math::Vec3 ta = -((edges[0] * dt[1]) - (edges[1] * dt[0])).normalized();
        tangents[tri[0]] += ta;
        tangents[tri[1]] += ta;
        tangents[tri[2]] += ta;
    }

    for(usize i = 0; i != tangents.size(); ++i) {
        Y_TODO(Compute bitangent too)
        vertices[i].packed_tangent_sign = pack_2_10_10_10(tangents[i].normalized());
    }

    return MeshData(std::move(vertices), copy(mesh.triangles()), copy(mesh.skin()), copy(mesh.bones()));
}


static AnimationChannel set_speed(const AnimationChannel& anim, float speed) {
    y_profile();
    auto keys = core::vector_with_capacity<AnimationChannel::BoneKey>(anim.keys().size());
    std::transform(anim.keys().begin(), anim.keys().end(), std::back_inserter(keys), [=](const auto& key){
            return AnimationChannel::BoneKey{key.time / speed, key.local_transform};
        });

    return AnimationChannel(anim.name(), std::move(keys));
}

static ImageData copy(const ImageData& image) {
    return ImageData(image.size().to<2>(), image.data(), image.format(), image.mipmaps());
}

Animation set_speed(const Animation& anim, float speed) {
    y_profile();
    auto channels = core::vector_with_capacity<AnimationChannel>(anim.channels().size());
    std::transform(anim.channels().begin(), anim.channels().end(), std::back_inserter(channels), [=](const auto& channel){ return set_speed(channel, speed); });

    return Animation(anim.duration() / speed, std::move(channels));
}


static void unpack_with_gamma(const u8* in, usize size, float* out) {
    y_profile();
    float gamma_lut[256];
    for(usize i = 0; i != 256; ++i) {
        gamma_lut[i] = std::pow(i / 255.0f, 2.2f);
    }

    for(usize i = 0; i != size; ++i) {
        out[i] = gamma_lut[in[i]];
        y_debug_assert(out[i] >= 0.0f);
        y_debug_assert(out[i] <= 1.0f);
    }
}

static void unpack(const u8* in, usize size, float* out) {
    y_profile();
    for(usize i = 0; i != size; ++i) {
        out[i] = in[i] / 255.0f;
        y_debug_assert(out[i] >= 0.0f);
        y_debug_assert(out[i] <= 1.0f);
    }
}

static void pack_with_gamma(const float* in, usize size, u8* out) {
    y_profile();
    const usize lut_size = 1 << 12;
    const float lut_factor = float(lut_size - 1);
    const float inv_lut_factor = 1.0f / lut_factor;
    const float inv_gamma = 1.0f / 2.2f;

    u8 gamma_lut[lut_size];
    for(usize i = 0; i != lut_size; ++i) {
        const float with_gamma = std::pow(i * inv_lut_factor, inv_gamma);
        y_debug_assert(with_gamma <= 1.0f);
        gamma_lut[i] = u8(with_gamma * 255.0f);
    }

#ifdef USE_SIMD
    const __m128 norm = _mm_set1_ps(lut_factor);

    y_always_assert(size % 4 == 0, "Size should be a multiple of 4");
    for(usize i = 0; i != size; i += 4) {
        const __m128 a = _mm_loadu_ps(in + i);
        const __m128 b = _mm_mul_ps(a, norm);
        const __m128 c = _mm_round_ps(b, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);    // round
        const __m128i d = _mm_cvtps_epi32(c);           // to int
        const u32* indexes = reinterpret_cast<const u32*>(&d);
        for(usize c = 0; c != 4; ++c) {
            out[i + c] = gamma_lut[indexes[c]];
        }
    }
#else
    for(usize i = 0; i != size; ++i) {
        const usize lut_index = usize(std::round(in[i] * lut_factor));
        y_debug_assert(lut_index < lut_size);
        out[i] = gamma_lut[lut_index];
    }
#endif
}

static void pack(const float* in, usize size, u8* out) {
    y_profile();

#ifdef USE_SIMD
    const char n = 15;
    const __m128 norm = _mm_set1_ps(255.0f);
    const __m128i mask = _mm_set_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 12, 8, 4, 0);

    y_always_assert(size % 4 == 0, "Size should be a multiple of 4");
    for(usize i = 0; i != size; i += 4) {
        const __m128 a = _mm_loadu_ps(in + i);
        const __m128 b = _mm_mul_ps(a, norm);
        const __m128 c = _mm_round_ps(b, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);    // round
        const __m128i d = _mm_cvtps_epi32(c);           // to int
        const __m128i e = _mm_shuffle_epi8(d, mask);    // extract bytes
        _mm_storeu_si32(out + i, e);                    // store
    }
#else
    for(usize i = 0; i < size; ++i) {
        y_debug_assert(in[i] >= 0.0f);
        y_debug_assert(in[i] <= 1.0f);
        out[i] = u8(std::round(in[i] * 255.0f));
    }
#endif
}

core::FixedArray<float> compute_mipmaps_internal(core::FixedArray<float> input, const math::Vec2ui& size, usize components, usize mip_count) {
    y_profile();

    y_debug_assert(size.x() * size.y() * components == input.size());

    const ImageFormat normalized_format = VK_FORMAT_R32G32B32A32_SFLOAT;
    const usize output_size = ImageData::layer_byte_size(math::Vec3ui(size, 1), normalized_format, mip_count) / sizeof(float);

    core::FixedArray<float> output(output_size);
    {
        const auto compute_mip = [&](const float* image_data, float* out, const math::Vec2ui& orig_size) -> usize {
            y_profile();

            usize cursor = 0;
            const math::Vec2ui mip_size = orig_size / 2;
            y_debug_assert(mip_size.min_component() != 0);

            const usize row_size = orig_size.x();

            for(usize y = 0; y != mip_size.y(); ++y) {
                for(usize x = 0; x != mip_size.x(); ++x) {
                    const usize orig = (x * 2 + y * 2 * row_size);
                    for(usize cc = 0; cc != components; ++cc) {
                        const float a = image_data[components * (orig) + cc];
                        const float b = image_data[components * (orig + 1) + cc];
                        const float c = image_data[components * (orig + row_size) + cc];
                        const float d = image_data[components * (orig + row_size + 1) + cc];
                        y_debug_assert((a + b + c + d) >= 0.0f);
                        out[cursor++] = std::min((a + b + c + d) * 0.25f, 1.0f);
                    }
                }
            }
            return cursor;
        };


        float* out_data = output.data();
        usize mip_values = size.x() * size.y() * components;
        std::copy_n(input.data(), mip_values, out_data);

        for(usize mip = 0; mip < mip_count - 1; ++mip) {
            const usize s = compute_mip(out_data, out_data + mip_values, ImageData::mip_size(math::Vec3ui(size, 1), mip).to<2>());
            out_data += mip_values;
            mip_values = s;
        }
    }

    return output;
}

ImageData compute_mipmaps(const ImageData& image) {
    y_profile();

    if(image.layers() != 1 || image.size().z() != 1) {
        log_msg("Unable to generate mipmaps: only one layer is supported.", Log::Error);
        return copy(image);
    }

    if(image.format().is_block_format() || image.format().is_depth_format()) {
        log_msg("Unable to generate mipmaps: format is not supported.", Log::Error);
        return copy(image);
    }

    const bool is_sRGB = image.format().is_sRGB();
    const usize components = image.format().components();
    const usize texels = image.size().x() * image.size().y();
    const usize mip_count = ImageData::mip_count(image.size());

    core::FixedArray<float> input(texels * components);
    {
        y_profile_zone("unpack");
        if(is_sRGB) {
            Y_TODO(Alpha should not be gammaed)
            unpack_with_gamma(image.data(), input.size(), input.data());
        } else {
            unpack(image.data(), input.size(), input.data());
        }
    }

    core::FixedArray<float> output = compute_mipmaps_internal(std::move(input), image.size().to<2>(), components, mip_count);
    core::FixedArray<u8> data(output.size());
    {
        y_profile_zone("pack");
        if(is_sRGB) {
            pack_with_gamma(output.data(), output.size(), data.data());
        } else {
            pack(output.data(), output.size(), data.data());
        }
    }
    output.clear();

    y_profile_zone("building image");
    return ImageData(image.size().to<2>(), data.data(), image.format(), mip_count);
}

}
}

