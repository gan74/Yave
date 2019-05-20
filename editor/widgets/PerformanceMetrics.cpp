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

#include "PerformanceMetrics.h"

#include <yave/device/Device.h>

#include <imgui/yave_imgui.h>

namespace editor {

PerformanceMetrics::PerformanceMetrics(ContextPtr cptr) : Widget("Performance", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
	std::fill(_frames.begin(), _frames.end(), 0.0f);
}

void PerformanceMetrics::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	auto time = _timer.reset();
	ImGui::Text("frame time: %.2fms", time.to_millis());

	_frames[_current_index] = time.to_millis();
	_current_index = (_current_index + 1) % _frames.size();

	ImGui::SetNextItemWidth(-1);
	ImGui::PlotLines("###graph", _frames.begin(), _frames.size(), _current_index, "", 0.0f, 50.0f, ImVec2(ImGui::GetWindowContentRegionWidth(), 80));


	ImGui::Text("%.3u resources waiting deletion", unsigned(device()->lifetime_manager().pending_deletions()));
	ImGui::Text("%.3u active command buffers", unsigned(device()->lifetime_manager().active_cmd_buffers()));
}

}
