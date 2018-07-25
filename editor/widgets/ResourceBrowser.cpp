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



ResourceBrowser::ResourceBrowser(ContextPtr ctx) :
		Widget("Resource browser"),
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
	node->children.clear();
	node->files.clear();

	const auto* fs = filesystem();
	auto dir = fs->join(node->path, node->name);
	fs->for_each(dir, [&](const auto& name) {
			auto full_name = fs->join(dir, name);
			if(fs->is_directory(full_name)) {
				node->children << DirNode(name, dir, node);
			} else {
				node->files << name;
			}
		});

	node->up_to_date = true;
}

static constexpr ImGuiTreeNodeFlags default_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

void ResourceBrowser::draw_node(DirNode* node, const core::String& name) {
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

void ResourceBrowser::paint_ui(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	unused(recorder, token);

	if(_update_chrono.elapsed().seconds() > update_secs) {
		_update_chrono.reset();
		update_node(_current);
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDarkening]);

	{
		float width = std::min(ImGui::GetWindowContentRegionWidth() * 0.5f, 200.0f);
		ImGui::BeginChild("###resourcestree", ImVec2(width, 0), false);
		draw_node(&_root, "store");
		ImGui::EndChild();
	}

	ImGui::SameLine();

	{
		ImGui::BeginChild("###resources");

		if(ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
			ImGui::OpenPopup("###resourcescontext");
		}

		if(ImGui::BeginPopupContextItem("###resourcescontext")) {
			if(ImGui::Selectable("Import mesh")) {
				MeshImporter* importer = context()->ui().add<MeshImporter>();
				importer->set_callback(
					[this, path = filesystem()->join(_current->path, _current->name)](auto meshes, auto anims) {
						save_meshes(path, meshes);
						save_anims(path, anims);
						update_node(_current);
					});
			}

			ImGui::EndPopup();
		}

		if(_current->parent && ImGui::Selectable("..")) {
			set_current(_current->parent);
		}

		auto curr = _current;
		for(DirNode& n : curr->children) {
			if(ImGui::Selectable(n.name.data())) {
				set_current(&n);
			}
		}

		for(const auto& name : curr->files) {
			if(ImGui::Selectable(name.data())) {
				try {
					auto full_name = filesystem()->join(_current->path, name);
					context()->scene().add(full_name);
				} catch(std::exception& e) {
					log_msg("Unable to add object to scene: "_s + e.what(), Log::Error);
				}
			}
		}
		ImGui::EndChild();
	}

	ImGui::PopStyleColor();
}


void ResourceBrowser::save_meshes(const core::String& path, core::ArrayView<Named<MeshData>> meshes) const {
	try {
		for(const auto& mesh : meshes) {
			core::String name = filesystem()->join(path, clean_name(mesh.name()) + ".ym");
			log_msg("Saving mesh as \"" + name + "\"");
			io::Buffer data;
			mesh.obj().serialize(data);
			context()->loader().asset_store().import(data, name);
		}
	} catch(std::exception& e) {
		log_msg("Unable save mesh: "_s + e.what(), Log::Error);
	}
}

void ResourceBrowser::save_anims(const core::String& path, core::ArrayView<Named<Animation>> anims) const {
	try {
		for(const auto& anim : anims) {
			core::String name = filesystem()->join(path, clean_name(anim.name()) + ".ya");
			log_msg("Saving animation as \"" + name + "\"");
			io::Buffer data;
			anim.obj().serialize(data);
			context()->loader().asset_store().import(data, name);
		}
	} catch(std::exception& e) {
		log_msg("Unable to save animation: "_s + e.what(), Log::Error);
	}
}

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->loader().asset_store().filesystem();
}

}
