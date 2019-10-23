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

#include "ui.h"
#include "assets.h"

#include <editor/context/EditorContext.h>
#include <editor/widgets/AssetSelector.h>

#include <yave/utils/FileSystemModel.h>

#include <imgui/yave_imgui.h>

namespace editor {
namespace imgui {

bool asset_selector(ContextPtr ctx, AssetId id, AssetType type, std::string_view text) {
	auto clean_name = [=](auto&& n) { return ctx->asset_store().filesystem()->filename(n); };

	core::String no_asset = fmt("No %", asset_type_name(type));
	core::String name = ctx->asset_store().name(id).map(clean_name).unwrap_or(no_asset);
	bool ret = ImGui::Button(fmt("%###%_%_%", ICON_FA_FOLDER_OPEN, id.id(), uenum(type), text.data()).data());
	ImGui::SameLine();
	ImGui::InputText(text.data(), name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
	return ret;
}

}
}
