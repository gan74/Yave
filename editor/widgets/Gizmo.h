/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <editor/ui/Frame.h>

namespace editor {

class Gizmo final : public Frame, public ContextLinked {
	public:
		Gizmo(ContextPtr cptr, SceneView* view);

		bool is_dragging() const;

	private:
		void paint_ui(CmdBufferRecorder&, const FrameToken&) override;

		math::Vec3 to_screen_pos(const math::Vec3& world);
		math::Vec2 to_window_pos(const math::Vec3& world);

		SceneView* _scene_view = nullptr;
		math::Vec3 _dragging_offset;
		u32 _dragging_mask = 0;
};

}

#endif // EDITOR_WIDGETS_GIZMO_H
