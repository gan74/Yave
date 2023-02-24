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
#ifndef EDITOR_WIDGETS_ENGINEVIEW_H
#define EDITOR_WIDGETS_ENGINEVIEW_H

#include "Gizmo.h"
#include "OrientationGizmo.h"

#include <editor/Widget.h>
#include <editor/renderer/EditorRenderer.h>

#include <yave/graphics/images/ImageView.h>
#include <yave/scene/SceneView.h>

#include <deque>

namespace editor {

class EngineView final : public Widget {

    editor_widget_open(EngineView, "View")

    public:
        enum class RenderView : u32 {
            Lit,
            Albedo,
            Normal,
            Metallic,
            Roughness,

            Depth,

            SSAO,
            ISMSplat,

            MaxRenderViews
        };

        EngineView();
        ~EngineView() override;

        CmdTimingRecorder* timing_recorder() const;

    protected:
        void on_gui() override;

        bool before_gui() override;
        void after_gui() override;

        bool should_keep_alive() const override;

    private:
        void draw(CmdBufferRecorder& recorder);
        void draw_menu_bar();
        void draw_settings_menu();
        void draw_gizmo_tool_bar();

        void update_proj();
        void update();
        void update_picking();

        bool is_mouse_inside() const;
        bool is_focussed() const;

        void make_drop_target();

        RenderView _view = RenderView::Lit;

        std::shared_ptr<FrameGraphResourcePool> _resource_pool;
        std::deque<std::unique_ptr<CmdTimingRecorder>> _time_recs;

        EditorRendererSettings _settings;

        SceneView _scene_view;
        std::unique_ptr<CameraController> _camera_controller;

        Gizmo _gizmo;
        OrientationGizmo _orientation_gizmo;

        bool _disable_render = false;
        isize _resolution = -1;
};

}

#endif // EDITOR_WIDGETS_ENGINEVIEW_H
