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

#include "MaterialEditor.h"
#include "AssetSelector.h"

#include <yave/material/Material.h>
#include <editor/context/EditorContext.h>

#include <y/io2/Buffer.h>

#include <editor/utils/ui.h>

#include <imgui/yave_imgui.h>

namespace editor {

MaterialEditor::MaterialEditor(ContextPtr cptr) :
		Widget(ICON_FA_BRUSH " Material Editor"),
		ContextLinked(cptr) {
}

static void modify_and_save(ContextPtr ctx, const AssetPtr<Material>& material, usize index, AssetId id) {
	auto tex = ctx->loader().load<Texture>(id);
	if(!tex) {
		log_msg("Unable to load texture.", Log::Error);
		return;
	}
	SimpleMaterialData data = material->data();
	data.set_texture(SimpleMaterialData::Textures(index), std::move(tex.unwrap()));

	io2::Buffer buffer;
	WritableAssetArchive ar(buffer);
	if(!data.serialize(ar)) {
		log_msg("Unable to serialize material.", Log::Error);
		return;
	}

	buffer.reset();
	if(!ctx->asset_store().write(material.id(), buffer)) {
		log_msg("Unable to write material.", Log::Error);
		return;
	}

	if(!ctx->loader().set(material.id(), Material(ctx->device(), std::move(data)))) {
		log_msg("Unable to reload material.", Log::Error);
		// force flush_reload anyway
	}

	ctx->flush_reload();
}

void MaterialEditor::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(!context()->selection().material()) {
		ImGui::Text("No material selected.");
		return;
	}

	AssetPtr<Material> material = context()->selection().material();
	const SimpleMaterialData& data = material->data();

	if(TextureView* view = context()->thumbmail_cache().get_thumbmail(material.id()).image) {
		math::Vec2 size = content_size().x() < view->size().x()
			? math::Vec2(content_size().x())
			: math::Vec2(view->size());
		ImGui::Image(view, size);
	}

	std::array<const char*, SimpleMaterialData::texture_count> texture_names = {"Diffuse", "Normal", "Roughness", "Metallic"};
	for(usize i = 0; i != data.textures().size(); ++i) {
		//ImGui::CollapsingHeader(texture_names[i], ImGuiTreeNodeFlags_DefaultOpen);

		if(imgui::asset_selector(context(), data.textures()[i].id(), AssetType::Image, texture_names[i])) {
			context()->ui().add<AssetSelector>(AssetType::Image)->set_selected_callback(
				[=, ctx = context()](AssetId id) {
					modify_and_save(ctx, material, i, id);
					return true;
				});
		}
	}
}

}
