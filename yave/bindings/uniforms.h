/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_BINDINGS_UNIFORMS_H
#define YAVE_BINDINGS_UNIFORMS_H

#include <yave/yave.h>
#include <yave/camera/Frustum.h>
#include <yave/objects/Light.h>

namespace yave {
namespace uniform {

using ViewProj = math::Matrix4<>;

struct Camera {
	math::Matrix4<> inv_matrix;
	math::Vec3 position;
	u32 padding;
};

using Frustum = yave::Frustum;

using LightType = Light::Type;

struct Light {
	math::Vec3 positon;
	float radius;
	math::Vec3 color;
	LightType type;
};

}
}

#endif // YAVE_BINDINGS_UNIFORMS_H
