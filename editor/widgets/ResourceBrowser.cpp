/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

static auto icon(u32 type) {
	switch(type) {
		case fs::image_file_type:
			return ICON_FA_IMAGE;

		case fs::mesh_file_type:
			return ICON_FA_CUBE;

		default:
		break;
	}
	return "";
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

u32 ResourceBrowser::file_type(const core::String& path) const {
	try {
		const AssetStore& store = context()->loader().asset_store();
		auto reader = store.data(store.id(path));
		u32 magic = reader->read_one<u32>();
		if(magic == fs::magic_number) {
			return reader->read_one<u32>();
		}
	} catch(std::exception&) {
	}

	return u32(-1);
}

void ResourceBrowser::update_node(DirNode* node) {
	node->children.clear();
	node->files.clear();

	const auto* fs = filesystem();
	auto dir = fs->join(node->path, node->name);
	fs->for_each(dir, [&](const auto& name) {
			auto full_name = fs->join(dir, name);
			if(fs->is_directory(full_name)) {
				node->children << DirNode(name, dir, node);
			} else {
				node->files << std::make_pair(name, file_type(full_name));
			}
		});

	node->up_to_date = true;
	_force_refresh = false;
}

void ResourceBrowser::draw_node(DirNode* node, const core::String& name) {
	static constexpr ImGuiTreeNodeFlags default_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
	ImGuiTreeNodeFlags flags = default_node_flags/* | (node->children.is_empty() ? ImGuiTreeNodeFlags_Leaf : 0)*/;
	if(ImGui::TreeNodeEx(name.data(), flags)) {
		if(!node->up_to_date) {
			update_node(node);
		}
		for(auto& n : node->children) {
			draw_node(&n, n.name);
		}
		ImGui::TreePop();
	}
}

void ResourceBrowser::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	unused(recorder, token);

	if(_force_refresh || _update_chrono.elapsed().seconds() > update_secs) {
		_update_chrono.reset();
		update_node(_current);
	}

	{
		float width = std::min(ImGui::GetWindowContentRegionWidth() * 0.5f, 200.0f);
		ImGui::BeginChild("###resourcestree", ImVec2(width, 0), false);
		draw_node(&_root, "store");
		ImGui::EndChild();
	}

	ImGui::SameLine();


	{
		ImGui::BeginChild("###resources");

		if(_current->parent && ImGui::Selectable("..")) {
			set_current(_current->parent);
		}

		auto curr = _current;
		for(DirNode& n : curr->children) {
			if(ImGui::Selectable(fmt("%", n.name).data())) {
				set_current(&n);
			}
		}

		bool context_menu = ImGui::IsPopupOpen("###resourcescontext");
		bool item_hovered = false;

		for(const auto& file : curr->files) {
			const auto& name = file.first;
			if(ImGui::Selectable(fmt("% %", icon(file.second), name).data())) {
				try {
					auto full_name = filesystem()->join(_current->path, name);
					context()->scene().add(full_name);
				} catch(std::exception& e) {
					log_msg(fmt("Unable to add object to scene: %", e.what()), Log::Error);
				}
			}

			if(!context_menu && ImGui::IsItemHovered()) {
				_hovered_asset = filesystem()->join(_current->path, name);
				item_hovered = true;
			}
		}

		if(!item_hovered && !context_menu) {
			_hovered_asset = "";
		}


		if(ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
			ImGui::OpenPopup("###resourcescontext");
		}

		if(ImGui::BeginPopup("###resourcescontext")) {
			if(_hovered_asset.is_empty()) {
				if(ImGui::Selectable("Import mesh")) {
					MeshImporter* importer = context()->ui().add<MeshImporter>();
					importer->set_callback(
						[this, path = filesystem()->join(_current->path, _current->name)](auto meshes, auto anims) {
							save_meshes(path, meshes);
							save_anims(path, anims);
							update_node(_current);
						});
				}
				if(ImGui::Selectable("Import image")) {
					ImageImporter* importer = context()->ui().add<ImageImporter>();
					importer->set_callback(
						[this, path = filesystem()->join(_current->path, _current->name)](auto images) {
							save_images(path, images);
							update_node(_current);
						});
				}
			} else {
				if(ImGui::Selectable("Rename")) {

					_force_refresh = true;
				}
				if(ImGui::Selectable("Delete")) {
					try {
						auto id = context()->loader().asset_store().id(_hovered_asset);
						context()->loader().asset_store().remove(id);
						_force_refresh = true;
					} catch(std::exception& e) {
						log_msg(fmt("Unable to add delete asset: %", e.what()), Log::Error);
					}
				}
			}

			ImGui::EndPopup();
		}

		ImGui::EndChild();
	}
}

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
