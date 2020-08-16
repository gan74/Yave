/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_ENGINEVIEW_H
#define EDITOR_ENGINEVIEW_H

#include <editor/ui/Widget.h>

#include <editor/widgets/Gizmo.h>
#include <editor/renderer/EditorRenderer.h>

#include <yave/ecs/ecs.h>

namespace editor {

class EngineView final : public Widget, public ContextLinked {

    public:
        enum class RenderView {
            Lit,
            Albedo,
            Normal,
            Metallic,
            Roughness,

            Depth,

            SSAO,

            MaxRenderViews
        };

        EngineView(ContextPtr cptr);
        ~EngineView() override;

    private:
        void paint(CmdBufferRecorder& recorder) override;

        void before_paint();
        void after_paint();

    private:
        static void draw_callback(RenderPassRecorder& recorder, void* user_data);

        void draw(CmdBufferRecorder& recorder);
        void draw_menu_bar();
        void draw_settings_menu();
        void draw_gizmo_tool_bar();

        bool is_clicked() const;

        void update_proj();
        void update();
        void update_picking();

        RenderView _view = RenderView::Lit;

        std::shared_ptr<FrameGraphResourcePool> _resource_pool;

        EditorRendererSettings _settings;

        SceneView _scene_view;
        std::unique_ptr<CameraController> _camera_controller;

        // subwidgets & stuff
        Gizmo _gizmo;

        bool _disable_render = false;
};

static_assert(!std::is_move_assignable_v<EngineView>);

}

#endif // EDITOR_ENGINEVIEW_H

