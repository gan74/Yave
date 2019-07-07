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

#include "ToolBar.h"

#include <editor/context/EditorContext.h>
#include <editor/EngineView.h>

#include <imgui/yave_imgui.h>

namespace editor {

ToolBar::ToolBar(ContextPtr ctx) : UiElement("Tool bar"), ContextLinked(ctx) {

}

void ToolBar::paint(CmdBufferRecorder&, const FrameToken&) {
	if(ImGui::BeginMenuBar()) {

		/*ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		if(ImGui::Button(ICON_FA_ARROWS_ALT)) {

		}
		ImGui::PushItemWidth(-100);
		if(ImGui::Button(ICON_FA_SYNC_ALT)) {

		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();*/

		Gizmo::Mode& gizmo_mode = context()->editor_state().gizmo_mode;
		Gizmo::Space& gizmo_space = context()->editor_state().gizmo_space;
		{

			if(ImGui::MenuItem(ICON_FA_ARROWS_ALT, nullptr, false, gizmo_mode != Gizmo::Translate)) {
				gizmo_mode = Gizmo::Translate;
			}
			if(ImGui::MenuItem(ICON_FA_SYNC_ALT, nullptr, false, gizmo_mode != Gizmo::Rotate)) {
				gizmo_mode = Gizmo::Rotate;
			}
		}
		{

			if(ImGui::MenuItem(ICON_FA_MOUNTAIN, nullptr, false, gizmo_space != Gizmo::World)) {
				gizmo_space = Gizmo::World;
			}
			if(ImGui::MenuItem(ICON_FA_CUBE, nullptr, false, gizmo_space != Gizmo::Local)) {
				gizmo_space = Gizmo::Local;
			}
		}

		if(const EngineView* view = context()->ui().find<EngineView>()) {
			if(view->is_focussed()) {
				const UiSettings& settings = context()->settings().ui();
				if(ImGui::IsKeyReleased(int(settings.change_gizmo_mode))) {
					gizmo_mode = Gizmo::Mode(!usize(gizmo_mode));
				}
				if(ImGui::IsKeyReleased(int(settings.change_gizmo_space))) {
					gizmo_space = Gizmo::Space(!usize(gizmo_space));
				}
			}
		}

		ImGui::EndMenuBar();
	}
}


}
