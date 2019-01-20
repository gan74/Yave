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

#include "ResourceBrowser.h"
#include "AssetRenamer.h"

#include <editor/context/EditorContext.h>
#include <y/io/Buffer.h>

#include <imgui/imgui.h>

namespace editor {

static core::String clean_name(std::string_view name) {
	if(name.empty()) {
		return "unnamed";
	}
	core::String str;
	str.set_min_capacity(name.size());
	for(char c : name) {
		str.push_back(std::isalnum(c) || c == '.' || c == '_' ? c : '_');
	}
	return str;
}

static AssetId asset_id(ContextPtr ctx, std::string_view name) {
	try {
		return ctx->asset_store().id(name);
	} catch(...) {
	}

	return AssetId();
}

static u32 read_file_type(ContextPtr ctx, AssetId id) {
	if(id.is_valid()) {
		try {
			const AssetStore& store = ctx->asset_store();
			auto reader = store.data(id);
			u32 magic = reader->read_one<u32>();
			if(magic == fs::magic_number) {
				return reader->read_one<u32>();
			}
		} catch(...) {
		}
	}
	return u32(-1);
}

static auto icon(u32 type) {
	switch(type) {
		case fs::image_file_type:
			return ICON_FA_IMAGE;

		case fs::mesh_file_type:
			return ICON_FA_CUBE;

		case fs::material_file_type:
			return ICON_FA_BRUSH;

		default:
			return ICON_FA_QUESTION;
	}
	return "";
}

ResourceBrowser::FileInfo::FileInfo(ContextPtr ctx, std::string_view file, std::string_view full) :
		name(file),
		full_name(full),
		id(asset_id(ctx, full)),
		file_type(read_file_type(ctx, id)) {
}


ResourceBrowser::DirNode::DirNode(std::string_view dir, std::string_view full, DirNode* par) :
		name(dir),
		full_path(full),
		parent(par) {
}




ResourceBrowser::ResourceBrowser(ContextPtr ctx) :
		Widget(ICON_FA_OBJECT_GROUP " Resource Browser"),
		ContextLinked(ctx),
		_root("", filesystem()->current_path()) {

	set_current(&_root);
}

void ResourceBrowser::set_current(DirNode* current) {
	if(!current) {
		set_current(&_root);
		return;
	}

	_current = current;
	update_node(_current);
}

void ResourceBrowser::update_node(DirNode* node) {
	reset_hover();
	node->children.clear();
	node->files.clear();

	const auto* fs = filesystem();
	fs->for_each(node->full_path, [&](const auto& name) {
			auto full_name = fs->join(node->full_path, name);
			if(fs->is_directory(full_name)) {
				node->children << DirNode(name, full_name, node);
			} else {
				node->files << FileInfo(context(), name, full_name);
			}
		});

	node->up_to_date = true;
	_refresh = false;
}

void ResourceBrowser::draw_node(DirNode* node, const core::String& name) {
	static constexpr ImGuiTreeNodeFlags default_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
	ImGuiTreeNodeFlags flags = default_node_flags/* | (node->children.is_empty() ? ImGuiTreeNodeFlags_Leaf : 0)*/;

	// folding/expending the node will still register as a goto somehow...
	if(ImGui::TreeNodeEx(fmt(ICON_FA_FOLDER " %", name).data(), flags)) {
		if(ImGui::IsItemClicked(0)) {
			set_current(node);
		}
		if(!node->up_to_date) {
			update_node(node);
		}
		for(auto& n : node->children) {
			draw_node(&n, n.name);
		}
		ImGui::TreePop();
	} else {
		if(ImGui::IsItemClicked(0)) {
			set_current(node);
		}
	}
}

void ResourceBrowser::refresh() {
	_refresh = true;
	//_current_file_index = usize(-1);
}

bool ResourceBrowser::need_refresh() const {
	return _refresh;
}

void ResourceBrowser::reset_hover() {
	_current_hovered_index = usize(-1);
}

const ResourceBrowser::FileInfo* ResourceBrowser::hovered_file() const {
	if(_current) {
		usize index = _current_hovered_index - _current->children.size();
		if(index < _current->files.size()) {
			return &_current->files[index];
		}
	}
	return nullptr;

}

const ResourceBrowser::DirNode* ResourceBrowser::hovered_dir() const {
	if(_current && _current_hovered_index < _current->children.size()) {
		return &_current->children[_current_hovered_index];
	}
	return nullptr;
}


// ----------------------------------- Context menu -----------------------------------

void ResourceBrowser::paint_context_menu() {
	if(ImGui::BeginPopup("###contextmenu")) {
		if(ImGui::Selectable("New folder")) {
			filesystem()->create_directory(filesystem()->join(_current->full_path, "new folder"));
			refresh();
		}

		if(const FileInfo* file = hovered_file()) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				context()->ui().add<AssetRenamer>(file->full_name);
			}
			if(ImGui::Selectable("Delete")) {
				try {
					context()->asset_store().remove(file->id);
				} catch(std::exception& e) {
					log_msg(fmt("Unable to delete asset: %", e.what()), Log::Error);
				}
				refresh();
			}
		} else if(const DirNode* dir = hovered_dir()) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				context()->ui().add<AssetRenamer>(dir->name);
			}
			if(ImGui::Selectable("Delete")) {
				try {
					context()->asset_store().remove(dir->full_path);
				} catch(std::exception& e) {
					log_msg(fmt("Unable to delete folder: %", e.what()), Log::Error);
				}
				refresh();
			}
		}

		ImGui::Separator();
		if(ImGui::Selectable("Import mesh")) {
			MeshImporter* importer = context()->ui().add<MeshImporter>();
			importer->set_callback(
				[this, path = _current->full_path](auto meshes, auto anims) {
					save_meshes(path, meshes);
					save_anims(path, anims);
					update_node(_current);
				});
		}
		if(ImGui::Selectable("Import image")) {
			ImageImporter* importer = context()->ui().add<ImageImporter>();
			importer->set_callback(
				[this, path = _current->full_path](auto images) {
					save_images(path, images);
					update_node(_current);
				});
		}
		ImGui::Separator();
		if(ImGui::Selectable("Create material")) {
			save_materials(_current->full_path, {Named("material", BasicMaterialData())});
			context()->selection().set_selected(device()->default_resources()[DefaultResources::BasicMaterial]);
			refresh();
		}

		if(FolderAssetStore* store = dynamic_cast<FolderAssetStore*>(&context()->asset_store())) {
			ImGui::Separator();
			if(ImGui::Selectable("Clean index")) {
				store->clean_index();
				refresh();
			}
		}

		ImGui::EndPopup();
	}
}


// ----------------------------------- Tree view -----------------------------------

void ResourceBrowser::paint_tree_view(float width) {
	ImGui::BeginChild("###resourcestree", ImVec2(width, 0), false);
	draw_node(&_root, "store");
	ImGui::EndChild();
}


// ----------------------------------- Asset list -----------------------------------

void ResourceBrowser::paint_asset_list(float width) {
	bool menu_openned = ImGui::IsPopupOpen("###contextmenu");

	ImGui::BeginChild("###resources", ImVec2(width, 0), false);

	// back
	{
		if(_current->parent && ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
			set_current(_current->parent);
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			reset_hover();
		}
	}

	usize index = 0;
	auto selected = [&] { return _current_hovered_index == index; };

	// dirs
	auto curr = _current;
	for(DirNode& n : curr->children) {
		if(ImGui::Selectable(fmt(ICON_FA_FOLDER " %", n.name).data(), selected())) {
			set_current(&n);
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			_current_hovered_index = index;
		}
		++index;
	}

	// files
	for(const auto& file : curr->files) {
		if(ImGui::Selectable(fmt("% %", icon(file.file_type), file.name).data(), selected())) {
			try {
				context()->scene().add(file.full_name);
			} catch(std::exception& e) {
				log_msg(fmt("Unable to add object to scene: %", e.what()), Log::Error);
			}
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			_current_hovered_index = index;
		}
		++index;
	}

	if(ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
		ImGui::OpenPopup("###contextmenu");
	}

	// this needs to be here?
	paint_context_menu();


	if(need_refresh() || (menu_openned && _update_chrono.elapsed() > update_duration)) {
		_update_chrono.reset();
		update_node(_current);
	}

	ImGui::EndChild();
}


// ----------------------------------- Preview -----------------------------------

void ResourceBrowser::paint_preview(float width) {
	if(const FileInfo* file = hovered_file()) {
		if(TextureView* image = context()->thumbmail_cache().get_thumbmail(file->id)) {
			ImGui::Image(image, math::Vec2(width));
		}

	}
}


// ----------------------------------- Main UI -----------------------------------

void ResourceBrowser::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	unused(recorder, token);

	const float width = ImGui::GetWindowContentRegionWidth();
	const float tree_width = std::min(width * 0.5f, 200.0f);
	const bool render_preview = width - tree_width > 400.0f;
	const float list_width = render_preview ? width - tree_width - 200.0f : 0.0f;
	const float preview_width = width - tree_width - list_width;

	paint_tree_view(tree_width);
	ImGui::SameLine();
	paint_asset_list(list_width);


	if(render_preview) {
		ImGui::SameLine();
		paint_preview(preview_width);
	}
}



// ----------------------------------- Imports -----------------------------------

template<typename T>
void ResourceBrowser::save_assets(const core::String& path, core::ArrayView<Named<T>> assets, const char* asset_name_type) const {
	try {
		for(const auto& a : assets) {
			core::String name = filesystem()->join(path, clean_name(a.name()));
			log_msg(fmt("Saving % as \"%\"", asset_name_type, name));
			io::Buffer data;
			serde::serialize(data, a.obj());
			context()->asset_store().import(data, name);
		}
	} catch(std::exception& e) {
		log_msg(fmt("Unable save %: %", asset_name_type, e.what()), Log::Error);
	}
}

void ResourceBrowser::save_images(const core::String& path, core::ArrayView<Named<ImageData>> images) const {
	save_assets(path, images, "image");
}

void ResourceBrowser::save_meshes(const core::String& path, core::ArrayView<Named<MeshData>> meshes) const {
	save_assets(path, meshes, "mesh");
}

void ResourceBrowser::save_anims(const core::String& path, core::ArrayView<Named<Animation>> anims) const {
	save_assets(path, anims, "animation");
}

void ResourceBrowser::save_materials(const core::String& path, core::ArrayView<Named<BasicMaterialData>> mats) const {
	save_assets(path, mats, "material");
}

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->asset_store().filesystem();
}

}
