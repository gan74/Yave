/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

using ViewProj = math::Matrix4<>;

static_assert(sizeof(ViewProj) % 16 == 0);


using Frustum = yave::Frustum;

static_assert(sizeof(Frustum) % 16 == 0);


struct DirectionalLight {
	math::Vec3 direction;
	u32 padding_0 = 0;

	math::Vec3 color;
	u32 padding_1 = 0;
};

static_assert(sizeof(DirectionalLight) % 16 == 0);


struct PointLight {
	math::Vec3 position;
	float radius = 1.0f;

	math::Vec3 color;
	float falloff = 1.0f;
};

static_assert(sizeof(PointLight) % 16 == 0);


struct SpotLight {
	math::Vec3 position;
	float radius = 1.0f;

	math::Vec3 color;
	float falloff = 1.0f;

	math::Vec3 forward;
	float cos_angle = 0.5f;

	math::Vec3ui padding_0;
	float angle_exp;
};

static_assert(sizeof(SpotLight) % 16 == 0);


struct LightingCamera {
	math::Matrix4<> inv_matrix;

	math::Vec3 position;
	u32 padding_0 = 0;

	math::Vec3 forward;
	u32 padding_1 = 0;
};

static_assert(sizeof(LightingCamera) % 16 == 0);


struct ToneMappingParams {
	float avg_luminance = 0.5f;
	float max_lum = 1.0f;

	math::Vec2ui padding_0;
};

static_assert(sizeof(ToneMappingParams) % 16 == 0);


struct RayleighSky {
	uniform::LightingCamera camera_data;
	math::Vec3 sun_direction;
	float origin_height;

	math::Vec3 sun_color;
	float planet_height;

	math::Vec3 beta_rayleigh;
	float atmo_height;
};

static_assert(sizeof(RayleighSky) % 16 == 0);


struct SH {
	std::array<math::Vec4, 9> values;
};

static_assert(sizeof(SH) % 16 == 0);


struct SkyLight {
	math::Vec3 sun_direction;
	u32 padding_0;

	math::Vec3 sun_color;
	u32 padding_1;

	SH sh;
};

static_assert(sizeof(SkyLight) % 16 == 0);

}
}

#endif // YAVE_GRAPHICS_DESCRIPTORS_UNIFORMS_H
