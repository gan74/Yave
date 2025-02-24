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

#include "image_utils.h"

#include <yave/graphics/images/ImageData.h>

#include <y/core/ScratchPad.h>
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

static ImageData copy(const ImageData& image) {
    return ImageData(image.size().to<2>(), image.data(), image.format(), image.mipmaps());
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
        const u32* indices = reinterpret_cast<const u32*>(&d);
        for(usize comp = 0; comp != 4; ++comp) {
            out[i + comp] = gamma_lut[indices[comp]];
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
    const usize output_size = ImageData::byte_size(math::Vec3ui(size, 1), normalized_format, mip_count) / sizeof(float);

    core::FixedArray<float> output(output_size);
    {
        const auto compute_mip = [&](const float* image_data, float* out, const math::Vec2ui& orig_size) -> usize {
            y_profile_zone("compute mip");

            usize cursor = 0;
            const math::Vec2ui mip_size = {
                std::max(1u, orig_size.x() / 2),
                std::max(1u, orig_size.y() / 2),
            };

            const math::Vec2 factors = math::Vec2(orig_size) / math::Vec2(mip_size);
            const float texel_width = factors.x() * 0.5f;
            const usize row_size = mip_size.y() > 1
                ? std::min(orig_size.x(), u32(orig_size.x() * texel_width))
                : 0u;

            for(usize y = 0; y != mip_size.y(); ++y) {
                for(usize x = 0; x != mip_size.x(); ++x) {
                    const usize orig = (u32(x * factors.x()) + u32(y * factors.y()) * row_size);
                    y_debug_assert(usize(orig + row_size + texel_width) <= orig_size.x() * orig_size.y());

                    for(usize cc = 0; cc != components; ++cc) {
                        const float a = image_data[components * usize(orig) + cc];
                        const float b = image_data[components * usize(orig + texel_width) + cc];
                        const float c = image_data[components * usize(orig + row_size) + cc];
                        const float d = image_data[components * usize(orig + row_size + texel_width) + cc];
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

    if(image.size().z() != 1) {
        log_msg("Unable to generate mipmaps: only one layer is supported", Log::Error);
        return copy(image);
    }

    const bool is_sRGB = image.format().is_sRGB();
    const usize components = image.format().components();
    const usize texels = image.size().x() * image.size().y();
    const usize mip_count = ImageData::mip_count(image.size());

    if(image.format().is_block_format() || image.format().is_depth_format() || image.format().bit_per_pixel() != 8 * components) {
        log_msg("Unable to generate mipmaps: format is not supported", Log::Error);
        return copy(image);
    }

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


template<typename F>
ImageData block_compress(const ImageData& image, ImageFormat compressed_format, F&& process_block) {
    if(image.format().bit_per_pixel() == 32 && image.size().z() == 1) {
        y_profile_zone("compress");

        const usize mip_count = image.mipmaps();
        const usize compressed_size = ImageData::byte_size(image.size(), compressed_format, mip_count);
        core::FixedArray<u8> compressed_data(compressed_size);

        const math::Vec3ui block_size = compressed_format.block_size();
        y_debug_assert(block_size.z() == 1);

        core::ScratchPad<u8> block(block_size.x() * block_size.y() * 4);

        usize offset = 0;
        for(usize i = 0; i != mip_count; ++i) {
            y_profile_zone("compress mip");
            const ImageData::Mip mip = image.mip_data(i);
            const usize mip_texel_count = mip.size.x() * mip.size.y();
            unused(mip_texel_count);

            for(usize y = 0; y < mip.size.y(); y += block_size.y()) {
                for(usize x = 0; x < mip.size.x(); x += block_size.x()) {

                    usize block_index = 0;
                    for(usize by = 0; by != block_size.y(); ++by) {
                        for(usize bx = 0; bx != block_size.x(); ++bx) {
                            const math::Vec2ui coord = math::Vec2ui(x + bx, y + by).min(mip.size.to<2>() - math::Vec2ui(1, 1));
                            const usize image_index = coord.y() * mip.size.x() + coord.x();
                            y_debug_assert(image_index < mip_texel_count);
                            for(usize c = 0; c != 4; ++c) {
                                block[block_index++] = u8(mip.data[image_index * 4 + c]);
                            }
                        }
                    }

                    const auto compressed_block = process_block(block.data());
                    y_debug_assert(offset + sizeof(compressed_block) <= compressed_data.size());
                    std::memcpy(compressed_data.data() + offset, &compressed_block, sizeof(compressed_block));
                    offset += sizeof(compressed_block);
                }
            }
        }

        y_debug_assert(offset == compressed_size);

        return ImageData(image.size().to<2>(), compressed_data.data(), compressed_format, mip_count);
    }

    log_msg("Compression isn't supported for given image format", Log::Warning);
    return copy(image);
}

static inline u16 to_565(u8 r, u8 g, u8 b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Inspired by https://github.com/wolfpld/tracy/blame/master/client/TracyDxt1.cpp
static inline u64 compress_block_bc1(const u8* src) {
    u8 min[3] = {src[0], src[1], src[2]};
    u8 max[3] = {src[0], src[1], src[2]};

    {
        const u8* next_px = src + 4;
        for(usize i = 1; i != 16; ++i) {
            for(usize c = 0; c != 3; ++c) {
                min[c] = std::min(min[c], next_px[c]);
                max[c] = std::max(max[c], next_px[c]);
            }
            next_px += 4;
        }
    }

#if 0
    for(usize c = 0; c != 3; ++c) {
        const u8 inset = (max[c] - min[c]) >> 4;
        min[c] += inset;
        max[c] -= inset;
        y_debug_assert(min[c] <= max[c]);
    }
#endif

    const u64 endpoint_0 = u64(to_565(min[0], min[1], min[2]));
    const u64 endpoint_1 = u64(to_565(max[0], max[1], max[2]));
    y_debug_assert(endpoint_0 <= endpoint_1);
    if(endpoint_0 == endpoint_1) {
        return endpoint_0;
    }

    math::Vec3i c0(min[0] & 0xF8, min[1] & 0xFC, min[2] & 0xF8);
    math::Vec3i c1(max[0] & 0xF8, max[1] & 0xFC, max[2] & 0xF8);
    const math::Vec3i colors[] = { c1, c0, (c1 * 2 + c0) / 3, (c1 + c0 * 2) / 3  };

    u32 idx_data = 0;
    for(usize i = 0; i != 16; ++i) {
        const math::Vec3i color(src[0] & 0xF8, src[1] & 0xFC, src[2] & 0xF8);
        u32 idx = 0;
        i32 best = (colors[0] - color).sq_length();
        for(u32 c = 1; c != 4; ++c) {
            i32 score = (colors[c] - color).sq_length();
            if(score < best) {
                best = score;
                idx = c;
            }
        }
        idx_data |= idx << (i * 2);
        src += 4;
    }

    return (u64(idx_data) << 32) | (endpoint_0 << 16) | (endpoint_1);
}

static inline u64 compress_block_bc4(const u8* src) {
    const usize stride = 4;
    u8 min = src[0];
    u8 max = src[0];

    for(usize i = 1; i != 16; ++i) {
        min = std::min(min, src[stride * i]);
        max = std::max(max, src[stride * i]);
    }

    const u8 diff = max - min;

    u64 idx_data = 0;
    for(usize i = 0; i != 16; ++i) {
        const u32 value = src[stride * i];
        const u64 idx = (value - min) * 8 / (diff + 1);
        y_debug_assert(idx < 8);
        idx_data |= idx << (i * 3);
        src += 4;
    }

    return (u64(idx_data) << 48) | (u64(min) << 8) | (u64(max));
}

ImageData compress_bc1(const ImageData& image) {
    const ImageFormat compressed_format = image.format().is_sRGB()
        ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
        : VK_FORMAT_BC1_RGB_UNORM_BLOCK;

    return block_compress(image, compressed_format, compress_block_bc1);
}

ImageData compress_bc4(const ImageData& image) {
    return block_compress(image, VK_FORMAT_BC4_UNORM_BLOCK, compress_block_bc4);
}

}
}

