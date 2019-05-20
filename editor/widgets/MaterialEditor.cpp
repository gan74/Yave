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

#include "MaterialEditor.h"
#include "AssetSelector.h"

#include <yave/material/Material.h>
#include <editor/context/EditorContext.h>

#include <y/io2/Buffer.h>

#include <imgui/yave_imgui.h>

namespace editor {

MaterialEditor::MaterialEditor(ContextPtr cptr) :
		Widget(ICON_FA_BRUSH " Material Editor"),
		ContextLinked(cptr) {
}

static void modify_and_save(ContextPtr ctx, const AssetPtr<Material>& material, usize index, AssetId id) {
	if(auto tex = ctx->loader().load<Texture>(id)) {
		SimpleMaterialData data = material->data();
		data.set_texture(SimpleMaterialData::Textures(index), std::move(tex.unwrap()));

		io2::Buffer buffer;
		WritableAssetArchive ar(buffer);
		if(data.serialize(ar)) {
			ctx->asset_store().write(material.id(), buffer).or_throw("");
			ctx->loader().set(material.id(), Material(ctx->device(), std::move(data))).or_throw("");

			ctx->flush_reload();
		} else {
			log_msg("Unable to save material.", Log::Error);
		}
	} else {
		log_msg("Unable to load texture.", Log::Error);
	}
}

void MaterialEditor::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(!context()->selection().material()) {
		ImGui::Text("No material selected.");
		return;
	}

	AssetPtr<Material> material = context()->selection().material();
	const SimpleMaterialData& data = material->data();
	math::Vec2 thumb_size = context()->thumbmail_cache().thumbmail_size() / 2;

	std::array<const char*, SimpleMaterialData::texture_count> texture_names = {"Diffuse", "Normal", "Roughness\nMetallic"};
	for(usize i = 0; i != data.textures().size(); ++i) {
		ImGui::PushID(fmt("%", i).data());

		TextureView* view = context()->thumbmail_cache().get_thumbmail(data.textures()[i].id());
		bool clicked = view
			? ImGui::ImageButton(view, thumb_size)
			: ImGui::Button(texture_names[i], thumb_size + math::Vec2(ImGui::GetStyle().FramePadding) * 2.0f);

		if(clicked) {
			AssetSelector* selector = context()->ui().add<AssetSelector>(AssetType::Image);
			selector->set_selected_callback([=, ctx = context()](AssetId id) {
				modify_and_save(ctx, material, i, id);
				return true;
			});
		}

		ImGui::PopID();
	}


	//MaterialTemplateData& data = const_cast<MaterialTemplateData&>(context()->selection().material()->data());
	//ImGui::Text("Binding count: %d", i32(data._bindings.size()));

	//ImGui::Checkbox("depth tested", &data._depth_tested);
}

}
