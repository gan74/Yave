/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include <yave/renderer/LightingPass.h>

namespace editor {

class EngineView final : public Widget, public ContextLinked {

	public:
		EngineView(ContextPtr cptr);
		~EngineView();

	private:
		void paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) override;

	private:
		static void draw_callback(RenderPassRecorder& recorder, void* user_data);

		void update();
		void update_camera();
		void update_selection();
		void update_picking();

		std::shared_ptr<IBLData> _ibl_data;

		SceneView _scene_view;

		// subwidgets & stuff
		Gizmo _gizmo;
		math::Vec3 _picked_pos;
};

static_assert(!std::is_move_assignable_v<EngineView>);

}

#endif // EDITOR_ENGINEVIEW_H
