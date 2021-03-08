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
#ifndef EDITOR_UTILS_CAMERACONTROLLER_H
#define EDITOR_UTILS_CAMERACONTROLLER_H

#include <editor/editor.h>

namespace editor {

struct PickingResult;

class CameraController : NonMovable {
    public:
        CameraController();

        virtual ~CameraController() = default;

        virtual bool viewport_clicked(const PickingResult& point) {
            unused(point);
            return false;
        }

        virtual void update_camera(Camera& camera, const math::Vec2ui& viewport_size) = 0;
        virtual void process_generic_shortcuts(Camera& camera);

};

class FPSCameraController final : public CameraController {
    public:
        FPSCameraController();

        void update_camera(Camera& camera, const math::Vec2ui& viewport_size) override;
};

class HoudiniCameraController final : public CameraController {
    public:
        HoudiniCameraController();

        bool viewport_clicked(const PickingResult& point) override;

        void update_camera(Camera& camera, const math::Vec2ui& viewport_size) override;

    private:
        int camera_key() const;

        math::Vec3 _picked_pos;
        math::Vec3 _target_offset;
        math::Vec3 _orig_pos;

        math::Vec2 _picking_uvs;
        math::Vec2 _cumulated_delta;

        float _picking_depth;

        bool _init = false;
        int _mouse_button = -1;
};


}

#endif // EDITOR_CAMERACONTROLLER_H

