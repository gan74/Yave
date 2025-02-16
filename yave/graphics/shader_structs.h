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
#ifndef YAVE_GRAPHICS_SHADER_STRUCTS_H
#define YAVE_GRAPHICS_SHADER_STRUCTS_H

#include <yave/yave.h>
#include <y/math/Vec.h>
#include <y/math/Matrix.h>

namespace yave {
namespace shader {

using uint = u32;

using float2 = math::Vec2;
using float3 = math::Vec3;
using float4 = math::Vec4;

using uint2 = math::Vec2ui;
using uint3 = math::Vec3ui;
using uint4 = math::Vec4ui;

using float4x4 = math::Matrix4<>;

#include <shaders/lib/structs.slang>

}

}


#endif // YAVE_GRAPHICS_SHADER_STRUCTS_H

