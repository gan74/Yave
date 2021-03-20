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
#ifndef YAVE_CAMERA_CAMERA_H
#define YAVE_CAMERA_CAMERA_H

#include <yave/graphics/descriptors/uniforms.h>
#include "Frustum.h"

namespace yave {

class Camera {

    public:
        Camera();

        void set_view(const math::Matrix4<>& view);
        void set_proj(const math::Matrix4<>& proj);

        const math::Matrix4<>& view_matrix() const;
        const math::Matrix4<>& proj_matrix() const;

        const math::Matrix4<>& viewproj_matrix() const;

        math::Matrix4<> inverse_matrix() const;

        float aspect_ratio() const;
        float field_of_view() const;

        math::Vec3 position() const;
        math::Vec3 forward() const;
        math::Vec3 right() const;
        math::Vec3 up() const;
        Frustum frustum() const;

        operator uniform::Camera() const;

    private:
        void update_viewproj() const;

        math::Matrix4<> _view;
        math::Matrix4<> _proj;

        Y_TODO(This is not thread safe)
        mutable math::Matrix4<> _viewproj;
        mutable bool _dirty = true;
};

}

#endif // YAVE_CAMERA_CAMERA_H

