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

#include <imgui/imgui.h>

namespace editor {

MaterialEditor::MaterialEditor(ContextPtr cptr) :
		Widget(ICON_FA_BRUSH " Material Editor"),
		ContextLinked(cptr) {
}

static void modify_and_save(ContextPtr ctx, const AssetPtr<Material>& material, usize index, AssetId id) {
	if(auto tex = ctx->loader().load<Texture>(id)) {
		std::array textures = material->data().textures();
		textures[index] = std::move(tex.unwrap());
		BasicMaterialData data(std::move(textures));

		try {
			io::Buffer buffer;
			data.serialize(buffer);
			ctx->asset_store().replace(buffer, material.id()).or_throw("");

			ctx->loader().forget(material.id());
			if(ctx->selection().material() == material) {
				if(auto mat = ctx->loader().load<Material>(material.id())) {
					ctx->selection().set_selected(mat.unwrap());
				}
			}
		} catch(...) {
			log_msg("Unable to save material.", Log::Error);
		}
		return;
	}
	log_msg("Unable to load texture.", Log::Error);
}

void MaterialEditor::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(!context()->selection().material()) {
		ImGui::Text("No material selected.");
		return;
	}

	AssetPtr<Material> material = context()->selection().material();
	const BasicMaterialData& data = material->data();
	math::Vec2ui thumb_size = context()->thumbmail_cache().thumbmail_size() / 2;

	for(usize i = 0; i != data.textures().size(); ++i) {
		TextureView* view = context()->thumbmail_cache().get_thumbmail(data.textures()[i].id());
		bool clicked = view
			? ImGui::ImageButton(view, thumb_size)
			: ImGui::Button(fmt("Texture###%", i).data(), thumb_size);

		if(clicked) {
			AssetSelector* selector = context()->ui().add<AssetSelector>(AssetType::Image);
			selector->set_selected_callback([=, ctx = context()](AssetId id) {
				modify_and_save(ctx, material, i, id);
				return true;
			});
		}
		ImGui::SameLine();
	}


	//MaterialTemplateData& data = const_cast<MaterialTemplateData&>(context()->selection().material()->data());
	//ImGui::Text("Binding count: %d", i32(data._bindings.size()));

	//ImGui::Checkbox("depth tested", &data._depth_tested);
}

}
