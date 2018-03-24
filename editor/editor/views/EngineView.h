/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef EDITOR_VIEWS_ENGINEVIEW_H
#define EDITOR_VIEWS_ENGINEVIEW_H

#include <editor/editor.h>

#include <editor/ui/Dock.h>

#include <editor/widgets/Gizmo.h>

#include <yave/renderers/FramebufferRenderer.h>
#include <yave/renderers/TiledDeferredRenderer.h>
#include <yave/scene/SceneView.h>
#include <yave/buffers/buffers.h>
#include <yave/material/Material.h>

namespace editor {

class EngineView : public Dock, public DeviceLinked {

		struct ViewData {
			math::Vec2i view_size;
			math::Vec2i view_offset;
			math::Vec2i render_size;
		};

	public:
		EngineView(DevicePtr dptr);

		void set_scene(Scene* scene);
		void set_selected(Transformable* tr);

	private:
		math::Vec2ui render_size() const;

		void paint_ui(CmdBufferRecorder<>& recorder, const FrameToken& token) override;

		bool set_render_size(math::Vec2ui size);

		void update_camera();

		static void draw_callback(RenderPassRecorder& recorder, void* user_data);

		void render_ui(RenderPassRecorder& recorder);
		void create_renderer(const math::Vec2ui& size);


		core::Unique<SceneView> _scene_view;

		Node::Ptr<IBLData> _ibl_data;
		Node::Ptr<FramebufferRenderer> _renderer;

		Node::Ptr<Material> _ui_material;
		TypedUniformBuffer<ViewData> _uniform_buffer;

		// subwidgets & stuff
		Gizmo _gizmo;
};

}

#endif // EDITOR_VIEWS_ENGINEVIEW_H
