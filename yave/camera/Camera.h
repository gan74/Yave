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
#ifndef YAVE_CAMERA_CAMERA_H
#define YAVE_CAMERA_CAMERA_H

#include <yave/graphics/shader_structs.h>
#include "Frustum.h"

namespace yave {

class Camera {
    public:
        static bool is_proj_orthographic(const math::Matrix4<>& proj);
        static float fov_from_proj(const math::Matrix4<>& proj);
        static float ratio_from_proj(const math::Matrix4<>& proj);

        static math::Vec3 forward_from_view(const math::Matrix4<>& view);
        static math::Vec3 right_from_view(const math::Matrix4<>& view);
        static math::Vec3 up_from_view(const math::Matrix4<>& view);
        static math::Vec3 position_from_view(const math::Matrix4<>& view);


        Camera();
        Camera(const math::Matrix4<>& view, const math::Matrix4<>& proj);

        void set_view(const math::Matrix4<>& view);
        void set_proj(const math::Matrix4<>& proj);

        void set_far(float far_dist);

        float aspect() const;

        math::Vec2 jitter() const;

        Camera jittered(math::Vec2 jitter_seq, const math::Vec2ui& size, float intensity = 1.0f) const;
        Camera unjittered() const;

        const math::Matrix4<>& view_matrix() const;
        const math::Matrix4<>& proj_matrix() const;

        const math::Matrix4<>& view_proj_matrix() const;

        math::Matrix4<> inverse_matrix() const;

        math::Matrix4<> unjittered_proj() const;
        math::Matrix4<> unjittered_view_proj() const;

        float aspect_ratio() const;
        float field_of_view() const;

        bool is_orthographic() const;

        float far_plane_dist() const;

        math::Vec3 position() const;
        math::Vec3 forward() const;
        math::Vec3 right() const;
        math::Vec3 up() const;
        Frustum frustum() const;

        operator shader::Camera() const;

    private:
        void update_view_proj();

        math::Matrix4<> _view;
        math::Matrix4<> _proj;
        math::Matrix4<> _view_proj;

        float _far = -1.0f;
};

}

#endif // YAVE_CAMERA_CAMERA_H

