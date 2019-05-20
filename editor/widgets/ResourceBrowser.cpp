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

#include <yave/assets/FolderAssetStore.h>

#include <y/io2/Buffer.h>

#include <imgui/yave_imgui.h>

#include <cinttypes>

namespace editor {

static AssetId asset_id(ContextPtr ctx, std::string_view name) {
	return ctx->asset_store().id(name).unwrap_or(AssetId());
}

static AssetType read_file_type(ContextPtr ctx, AssetId id) {
	return ctx->asset_store().asset_type(id).unwrap_or(AssetType::Unknown);
}

ResourceBrowser::FileInfo::FileInfo(ContextPtr ctx, std::string_view file, std::string_view full) :
		name(file),
		full_name(full),
		id(asset_id(ctx, full)),
		type(read_file_type(ctx, id)) {
}

ResourceBrowser::DirNode::DirNode(std::string_view dir, std::string_view full, DirNode* par) :
		name(dir),
		full_path(full),
		parent(par) {
}



ResourceBrowser::ResourceBrowser(ContextPtr ctx) : ResourceBrowser(ctx, ICON_FA_FOLDER_OPEN " Resource Browser") {
}

ResourceBrowser::ResourceBrowser(ContextPtr ctx, std::string_view title) :
		Widget(title, ImGuiWindowFlags_AlwaysVerticalScrollbar),
		ContextLinked(ctx),
		_root("", filesystem()->current_path().unwrap_or("")) {

	set_current(&_root);
}

void ResourceBrowser::asset_selected(const FileInfo& file) {
	switch(file.type) {
		case AssetType::Material:
			if(auto material = context()->loader().load<Material>(file.id)) {
				context()->selection().set_selected(material.unwrap());
			} else {
				log_msg("Unable to open material.", Log::Error);
			}
		break;

		default:
		break;
	}
}

bool ResourceBrowser::display_asset(const FileInfo& file) const {
	return file.type != AssetType::Unknown;
}

std::string_view ResourceBrowser::icon_for_type(AssetType type) {
	switch(type) {
		case AssetType::Image:
			return ICON_FA_IMAGE;

		case AssetType::Mesh:
			return ICON_FA_CUBE;

		case AssetType::Material:
			return ICON_FA_BRUSH;

		default:
			return ICON_FA_QUESTION;
	}
	return ICON_FA_QUESTION;
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
	y_profile();

	reset_hover();
	node->children.clear();
	node->files.clear();

	const auto* fs = filesystem();
	fs->for_each(node->full_path, [&](const auto& name) {
			auto full_name = fs->join(node->full_path, name);
			if(fs->is_directory(full_name).unwrap_or(false)) {
				node->children << DirNode(name, full_name, node);
			} else {
				node->files << FileInfo(context(), name, full_name);
			}
		}).ignore();

	node->up_to_date = true;
	_refresh = false;
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
			filesystem()->create_directory(filesystem()->join(_current->full_path, "new folder")).ignore();
			refresh();
		}

		if(const FileInfo* file = hovered_file()) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				context()->ui().add<AssetRenamer>(file->full_name);
			}
			if(ImGui::Selectable("Delete")) {
				if(!context()->asset_store().remove(file->id)) {
					log_msg("Unable to delete asset", Log::Error);
				}
				refresh();
			}
		} else if(const DirNode* dir = hovered_dir()) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				context()->ui().add<AssetRenamer>(dir->name);
			}
			if(ImGui::Selectable("Delete")) {
				if(!context()->asset_store().remove(dir->full_path)) {
					log_msg("Unable to delete folder", Log::Error);
				}
				refresh();
			}
		}

		ImGui::Separator();
		if(ImGui::Selectable("Import mesh")) {
			context()->ui().add<MeshImporter>(_current->full_path);
		}
		if(ImGui::Selectable("Import image")) {
			context()->ui().add<ImageImporter>(_current->full_path);
		}
		ImGui::Separator();
		if(ImGui::Selectable("Create material")) {
			SimpleMaterialData material;
			io2::Buffer buffer;
			WritableAssetArchive ar(buffer);
			if(material.serialize(ar)) {
				AssetStore& store = context()->asset_store();
				if(!store.import(buffer, store.filesystem()->join(_current->full_path, "new material"))) {
					log_msg("Unable to import new material.", Log::Error);
				}
			} else {
				log_msg("Unable to create new material.", Log::Error);
			}

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
	usize hovered = usize(-1);
	auto selected = [&] { return _current_hovered_index == index; };

	// dirs
	auto curr = _current;
	for(DirNode& n : curr->children) {
		if(ImGui::Selectable(fmt(ICON_FA_FOLDER " %", n.name).data(), selected())) {
			set_current(&n);
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			hovered = index;
		}
		++index;
	}

	// files
	for(const FileInfo& file : curr->files) {
		if(display_asset(file)) {
			if(ImGui::Selectable(fmt("% %", icon_for_type(file.type), file.name).data(), selected())) {
				asset_selected(file);
			}

			if(!menu_openned && ImGui::IsItemHovered()) {
				hovered = index;
			}
		}
		++index;
	}

	if(!ImGui::IsPopupOpen("###contextmenu")) {
		_current_hovered_index = hovered;
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
		ImGui::BeginGroup();

		if(TextureView* image = context()->thumbmail_cache().get_thumbmail(file->id)) {
			ImGui::Image(image, math::Vec2(width));
		}
		ImGui::Text("ID: 0x%.8x", unsigned(file->id.id()));

		ImGui::EndGroup();
	}
}


// ----------------------------------- Main UI -----------------------------------

void ResourceBrowser::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	unused(recorder, token);
	y_profile();

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

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->asset_store().filesystem();
}

}
