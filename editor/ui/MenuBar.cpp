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

#include "MenuBar.h"

#include <editor/context/EditorContext.h>

#include <editor/widgets/EntityView.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/MemoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/MaterialEditor.h>
#include <editor/widgets/AssetStringifier.h>
#include <editor/widgets/Console.h>

#include <editor/properties/PropertyPanel.h>
#include <editor/EngineView.h>

#include <imgui/yave_imgui.h>

namespace editor {

MenuBar::MenuBar(ContextPtr ctx) : UiElement("Menu bar"), ContextLinked(ctx) {
}

void MenuBar::paint(CmdBufferRecorder&, const FrameToken&) {
	if(ImGui::BeginMenuBar()) {
		if(ImGui::BeginMenu(ICON_FA_FILE " File")) {

			if(ImGui::MenuItem(ICON_FA_FILE " New")) {
				context()->new_world();
			}

			ImGui::Separator();

			if(ImGui::MenuItem(ICON_FA_SAVE " Save")) {
				context()->defer([ctx = context()] { ctx->save_world(); });
			}

			if(ImGui::MenuItem(ICON_FA_FOLDER " Load")) {
				context()->load_world();
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("View")) {
			if(ImGui::MenuItem("Engine view")) context()->ui().add<EngineView>();
			if(ImGui::MenuItem("Entity view")) context()->ui().add<EntityView>();
			if(ImGui::MenuItem("Resource browser")) context()->ui().add<ResourceBrowser>();
			if(ImGui::MenuItem("Material editor")) context()->ui().add<MaterialEditor>();
			ImGui::Separator();
			if(ImGui::MenuItem("Console")) context()->ui().add<Console>();

			ImGui::Separator();

			if(ImGui::BeginMenu("Debug")) {
				if(ImGui::MenuItem("Camera debug")) context()->ui().add<CameraDebug>();

				ImGui::Separator();
				if(ImGui::MenuItem("Asset stringifier")) context()->ui().add<AssetStringifier>();

				ImGui::Separator();
				if(ImGui::MenuItem("Flush reload")) context()->flush_reload();

				y_debug_assert(!(ImGui::Separator(), ImGui::MenuItem("Debug assert")));

				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Statistics")) {
				if(ImGui::MenuItem("Performances")) context()->ui().add<PerformanceMetrics>();
				if(ImGui::MenuItem("Memory info")) context()->ui().add<MemoryInfo>();
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if(ImGui::MenuItem(ICON_FA_COG " Settings")) context()->ui().add<SettingsPanel>();

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Tools")) {
			if(ImGui::MenuItem("Reload resources")) context()->reload_device_resources();
			ImGui::EndMenu();
		}

#ifdef Y_PERF_LOG_ENABLED
		if(perf::is_capturing()) {
			ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
			if(ImGui::MenuItem(ICON_FA_STOPWATCH)) context()->end_perf_capture();
			ImGui::PopStyleColor();
		} else {
			if(ImGui::MenuItem(ICON_FA_STOPWATCH)) context()->start_perf_capture();
		}
#endif

		ImGui::EndMenuBar();
	}
}

}
