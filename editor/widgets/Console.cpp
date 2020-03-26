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

#include "Console.h"

#include <editor/context/EditorContext.h>

#include <imgui/yave_imgui.h>

#include <numeric>

namespace editor {

Console::Console(ContextPtr cptr) :
		Widget(ICON_FA_TERMINAL " Console"),
		ContextLinked(cptr) {


	std::fill(_filter.begin(), _filter.end(), 0);
	std::fill(_log_types.begin(), _log_types.end(), true);
	std::fill(_log_counts.begin(), _log_counts.end(), 0);
}



static ImVec4 log_color(Log type) {
	switch(type) {
		case Log::Error:
			return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

		case Log::Warning:
			return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);

		case Log::Perf:
			return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

		default:
			break;
	}
	return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

static const char* log_icon(Log type) {
	switch(type) {
		case Log::Info:
			return ICON_FA_INFO_CIRCLE;

		case Log::Error:
			return ICON_FA_TIMES_CIRCLE;

		case Log::Warning:
			return ICON_FA_EXCLAMATION_CIRCLE;

		case Log::Debug:
			return ICON_FA_QUESTION_CIRCLE;

		case Log::Perf:
			return ICON_FA_CLOCK;
	}
	y_fatal("Unknown log type.");
}

static bool match(const char* str, const char* needle) {
	for(;; ++str) {
		if(!*str) {
			return false;
		}
		for(usize i = 0;; ++i) {
			if(!needle[i]) {
				return true;
			}
			if(std::tolower(needle[i]) != std::tolower(str[i])) {
				break;
			}
		}
	}
}


void Console::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	y_profile();

	ImGui::SetNextItemWidth(-260.0f);
	ImGui::InputText("Filter", _filter.data(), _filter.size());

	ImGui::SameLine();
	if(ImGui::Button("Clear")) {
		context()->logs().clear();
	}

	const usize total = std::accumulate(_log_counts.begin(), _log_counts.end(), usize(0));
	for(usize i = 0; i != log_type_count; ++i) {
		ImGui::PushStyleColor(ImGuiCol_Text, _log_types[i] ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		ImGui::SameLine();
		_log_types[i] ^= ImGui::Button(log_icon(Log(i)));
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(fmt_c_str("% / %", _log_counts[i], total));
		}
	}

	/*{
		ImGui::SameLine();
		if(ImGui::Button("Test")) {
			log_msg("error", Log::Error);
			log_msg("warn", Log::Warning);
			log_msg("perf", Log::Perf);
			log_msg("info", Log::Info);
			log_msg("debug", Log::Debug);
		}
	}*/


	ImGui::BeginChild("###console", ImVec2(), true, ImGuiWindowFlags_HorizontalScrollbar);

	std::fill(_log_counts.begin(), _log_counts.end(), 0);
	context()->logs().for_each([&](const Logs::Message& msg) {
			++_log_counts[usize(msg.type)];
			if(!_log_types[usize(msg.type)]) {
				return;
			}
			if(!_filter.front() || match(msg.msg.data(), _filter.data())) {
				ImGui::PushStyleColor(ImGuiCol_Text, log_color(msg.type));
				ImGui::Selectable(fmt_c_str("% %", log_icon(msg.type), msg.msg));
				ImGui::PopStyleColor();
			}
		});

	_auto_scroll = std::abs(ImGui::GetScrollMaxY() - ImGui::GetScrollY()) < 1.0f;
	if(_auto_scroll) {
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
}

}
