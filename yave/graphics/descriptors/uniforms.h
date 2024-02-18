/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_UNIFORMS_H
#define YAVE_GRAPHICS_DESCRIPTORS_UNIFORMS_H

#include <yave/yave.h>
#include <yave/camera/Frustum.h>

namespace yave {
namespace uniform {


struct Camera {
    math::Matrix4<> view_proj;
    math::Matrix4<> inv_view_proj;

    math::Matrix4<> unjittered_view_proj;
    math::Matrix4<> inv_unjittered_view_proj;

    math::Matrix4<> prev_unjittered_view_proj;

    math::Matrix4<> proj;
    math::Matrix4<> inv_proj;

    math::Matrix4<> view;
    math::Matrix4<> inv_view;

    math::Vec3 position;
    u32 padding_0 = 0;

    math::Vec3 forward;
    u32 padding_1 = 0;

    math::Vec3 up;
    u32 padding_2 = 0;
};

static_assert(sizeof(Camera) % 16 == 0);


struct TransformableData {
    math::Matrix4<> current;
    math::Matrix4<> last;
};

static_assert(sizeof(TransformableData) % 16 == 0);


struct MaterialData {
    static constexpr usize texture_count = 8;

    math::Vec3 emissive_mul;
    float roughness_mul = 1.0f;

    math::Vec3 base_color_mul;
    float metallic_mul = 0.0f;

    u32 texture_indices[texture_count];
};

static_assert(sizeof(MaterialData) % 16 == 0);


struct DirectionalLight {
    math::Vec3 direction;
    float cos_disk = 0.0f;

    math::Vec3 color;
    u32 padding_1 = 0;

    math::Vec4ui shadow_map_indices;
};

static_assert(sizeof(DirectionalLight) % 16 == 0);


struct PointLight {
    math::Vec3 position;
    float range = 1.0f;

    math::Vec3 color;
    float falloff = 1.0f;

    math::Vec3 padding_0 = {};
    float min_radius = 0.01f;
};

static_assert(sizeof(PointLight) % 16 == 0);


struct SpotLight {
    math::Vec3 position;
    float range = 1.0f;

    math::Vec3 color;
    float falloff = 1.0f;

    math::Vec3 forward;
    float min_radius = 0.01f;

    math::Vec2 att_scale_offset;
    float sin_angle = 0.0f;
    u32 shadow_map_index = u32(-1);

    math::Vec3 encl_sphere_center;
    float encl_sphere_radius;
};

static_assert(sizeof(SpotLight) % 16 == 0);


struct ShadowMapParams {
    math::Matrix4<> view_proj;

    math::Vec2 uv_offset;
    math::Vec2 uv_mul;

    float size;
    float texel_size;
    u32 padding_0;
    u32 padding_1;
};

static_assert(sizeof(ShadowMapParams) % 16 == 0);


struct ExposureParams {
    float exposure = 1.0f;
    float avg_luminance = 0.5f;
    float max_lum = 0.5f * 9.6f;

    u32 padding_0;
};

static_assert(sizeof(ExposureParams) % 16 == 0);


struct AtmosphereParams {
    math::Vec3 center;
    float planet_radius;

    math::Vec3 scattering_coeffs;
    float atmosphere_height;

    math::Vec3 light_dir;
    float radius; // planet_radius + atmosphere_height

    math::Vec3 light_color;
    float density_falloff;
};

static_assert(sizeof(AtmosphereParams) % 16 == 0);


}
}

#endif // YAVE_GRAPHICS_DESCRIPTORS_UNIFORMS_H

