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

#include "MaterialEditor.h"
#include "AssetSelector.h"

#include <editor/context/EditorContext.h>

#include <yave/renderer/renderer.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/entities/entities.h>
#include <yave/utils/color.h>

#include <y/io2/Buffer.h>

#include <editor/utils/ui.h>

#include <imgui/yave_imgui.h>

namespace editor {

static void modify_and_save(ContextPtr ctx, AssetPtr<Material>& material, usize index, AssetId id) {
	y_profile();
	const auto tex = ctx->loader().load_res<Texture>(id);
	if(!tex) {
		log_msg("Unable to load texture.", Log::Error);
		return;
	}
	SimpleMaterialData data = material->data();
	data.set_texture(SimpleMaterialData::Textures(index), std::move(tex.unwrap()));

	io2::Buffer buffer;
	serde3::WritableArchive arc(buffer);
	if(!arc.serialize(data)) {
		log_msg("Unable to serialize material.", Log::Error);
		return;
	}

	buffer.reset();
	if(!ctx->asset_store().write(material.id(), buffer)) {
		log_msg("Unable to write material.", Log::Error);
		return;
	}

	{
		y_profile_zone("reload");
		material.reload();
		ctx->flush_reload();
	}
}

MaterialEditor::MaterialEditor(ContextPtr cptr) :
		Widget(ICON_FA_BRUSH " Material Editor"),
		ContextLinked(cptr),
		_preview(cptr) {

	_preview.set_parent(this);
}

void MaterialEditor::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();

	if(_material) {
		_preview.paint(recorder, token);
	}

	if(imgui::asset_selector(context(), _material.id(), AssetType::Material, "Material")) {
		add_child<AssetSelector>(context(), AssetType::Material)->set_selected_callback(
			[this](AssetId asset) {
				if(const auto mat = context()->loader().load_res<Material>(asset)) {
					_material = mat.unwrap();
					_preview.set_material(_material);
				}
				return true;
			});
	}

	if(!_material) {
		return;
	}

	const SimpleMaterialData& data = _material->data();

	const std::array<const char*, SimpleMaterialData::texture_count> texture_names = {"Diffuse", "Normal", "Roughness", "Metallic"};
	for(usize i = 0; i != data.textures().size(); ++i) {
		//ImGui::CollapsingHeader(texture_names[i], ImGuiTreeNodeFlags_DefaultOpen);

		if(imgui::asset_selector(context(), data.textures()[i].id(), AssetType::Image, texture_names[i])) {
			add_child<AssetSelector>(context(), AssetType::Image)->set_selected_callback(
				[=, ctx = context()](AssetId id) {
					modify_and_save(ctx, _material, i, id);
					return true;
				});
		}
	}
}

}
