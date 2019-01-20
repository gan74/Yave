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
		return ctx->loader().asset_store().id(name);
	} catch(...) {
	}

	return AssetId();
}


static u32 read_file_type(ContextPtr ctx, AssetId id) {
	if(id.is_valid()) {
		try {
			const AssetStore& store = ctx->loader().asset_store();
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

		default:
			return ICON_FA_QUESTION;
	}
	return "";
}



ResourceBrowser::FileInfo::FileInfo(ContextPtr ctx, std::string_view filename) :
		name(filename),
		id(asset_id(ctx, name)),
		file_type(read_file_type(ctx, id)) {
}


ResourceBrowser::DirNode::DirNode(std::string_view n, std::string_view p, DirNode* par) :
		name(n),
		path(p),
		parent(par) {
}

const ResourceBrowser::FileInfo* ResourceBrowser::DirNode::file_at(usize index) const {
	index -= children.size();
	if(index < files.size()) {
		return &files[index];
	}
	return nullptr;
}





ResourceBrowser::ResourceBrowser(ContextPtr ctx) :
		Widget(ICON_FA_OBJECT_GROUP " Resource browser"),
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
	_current_file = nullptr;
	node->children.clear();
	node->files.clear();

	const auto* fs = filesystem();
	fs->for_each(node->path, [&](const auto& name) {
			auto full_name = fs->join(node->path, name);
			if(fs->is_directory(full_name)) {
				node->children << DirNode(name, full_name, node);
			} else {
				node->files << FileInfo(context(), name);
			}
		});

	node->up_to_date = true;
	_force_refresh = false;
}

void ResourceBrowser::draw_node(DirNode* node, const core::String& name) {
	static constexpr ImGuiTreeNodeFlags default_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
	ImGuiTreeNodeFlags flags = default_node_flags/* | (node->children.is_empty() ? ImGuiTreeNodeFlags_Leaf : 0)*/;
	if(ImGui::TreeNodeEx(fmt(ICON_FA_FOLDER " %", name).data(), flags)) {
		if(!node->up_to_date) {
			update_node(node);
		}
		for(auto& n : node->children) {
			draw_node(&n, n.name);
		}
		ImGui::TreePop();
	}
}



// ----------------------------------- Context menu -----------------------------------

bool ResourceBrowser::context_menu_opened() const {
	return ImGui::IsPopupOpen("###contextmenu");
}

void ResourceBrowser::paint_context_menu() {
	if(ImGui::BeginPopup("###contextmenu")) {
		if(ImGui::Selectable("New folder")) {
			filesystem()->create_directory(filesystem()->join(_current->path, "new folder"));
			_force_refresh = true;
		}

		if(_current_file) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				_force_refresh = true;
			}
			if(ImGui::Selectable("Delete")) {
				try {
					context()->loader().asset_store().remove(_current_file->id);
				} catch(std::exception& e) {
					log_msg(fmt("Unable to add delete asset: %", e.what()), Log::Error);
				}
				_force_refresh = true;
			}
		}

		ImGui::Separator();
		if(ImGui::Selectable("Import mesh")) {
			MeshImporter* importer = context()->ui().add<MeshImporter>();
			importer->set_callback(
				[this, path = _current->path](auto meshes, auto anims) {
					save_meshes(path, meshes);
					save_anims(path, anims);
					update_node(_current);
				});
		}
		if(ImGui::Selectable("Import image")) {
			ImageImporter* importer = context()->ui().add<ImageImporter>();
			importer->set_callback(
				[this, path = _current->path](auto images) {
					save_images(path, images);
					update_node(_current);
				});
		}
		ImGui::Separator();
		if(ImGui::Selectable("Create material")) {
			context()->selection().set_selected(device()->default_resources()[DefaultResources::BasicMaterial]);
		}

		if(FolderAssetStore* store = dynamic_cast<FolderAssetStore*>(&context()->loader().asset_store())) {
			ImGui::Separator();
			if(ImGui::Selectable("Clean index")) {
				store->clean_index();
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
	bool menu_openned = context_menu_opened();

	ImGui::BeginChild("###resources", ImVec2(width, 0), false);

	if(_current->parent && ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
		set_current(_current->parent);
	}

	// dirs
	auto curr = _current;
	for(DirNode& n : curr->children) {
		if(ImGui::Selectable(fmt(ICON_FA_FOLDER " %", n.name).data())) {
			set_current(&n);
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			_current_file = nullptr;
		}
	}

	// files
	for(const auto& file : curr->files) {
		const auto& name = file.name;
		if(ImGui::Selectable(fmt("% %", icon(file.file_type), name).data())) {
			try {
				auto full_name = filesystem()->join(_current->path, name);
				context()->scene().add(full_name);
			} catch(std::exception& e) {
				log_msg(fmt("Unable to add object to scene: %", e.what()), Log::Error);
			}
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			_current_file = &file;
		}
	}

	if(ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
		ImGui::OpenPopup("###contextmenu");
	}

	// this needs to be here?
	paint_context_menu();

	ImGui::EndChild();
}


// ----------------------------------- Preview -----------------------------------

void ResourceBrowser::paint_preview(float width) {
	if(_current_file) {
		if(TextureView* image = context()->thumbmail_cache().get_thumbmail(_current_file->id)) {
			ImGui::Image(image, math::Vec2(width));
		}
	}
}


// ----------------------------------- Main UI -----------------------------------

void ResourceBrowser::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	unused(recorder, token);

	if(_force_refresh || _update_chrono.elapsed() > update_duration) {
		_update_chrono.reset();
		update_node(_current);
	}

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
			a.obj().serialize(data);
			context()->loader().asset_store().import(data, name);
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

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->loader().asset_store().filesystem();
}

}
