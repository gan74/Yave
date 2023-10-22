/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef EDITOR_WIDGETS_GIZMO_H
#define EDITOR_WIDGETS_GIZMO_H

#include <editor/editor.h>

#include <yave/ecs/ecs.h>

namespace editor {

class GizmoBase {
    public:
        GizmoBase(SceneView* view);
        virtual ~GizmoBase();

        void set_allow_dragging(bool allow);
        bool is_dragging() const;

        virtual void draw() = 0;

    protected:
        math::Vec3 to_screen_pos(const math::Vec3& world) const;
        math::Vec2 to_window_pos(const math::Vec3& world) const;
        math::Vec3 to_world_pos(const math::Vec2& window) const;

        SceneView* _scene_view = nullptr;

        bool _is_dragging = false;
        bool _allow_dragging = true;
};

class TranslationGizmo final : public GizmoBase {
    public:
        TranslationGizmo(SceneView* view);

        void draw() override;

    private:
        bool is_hovered(usize axis) const;

        u32 _hover_mask = 0;
        math::Vec2 _dragging_offset;
};


class RotationGizmo final : public GizmoBase {
    public:
        RotationGizmo(SceneView* view);

        void draw() override;

    private:
        usize _rotation_axis = usize(-1);
        float _angle_offset = 0.0f;

};

}

#endif // EDITOR_WIDGETS_GIZMO_H

