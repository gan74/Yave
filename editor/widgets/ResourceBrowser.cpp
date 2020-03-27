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

#include "ResourceBrowser.h"
#include "AssetRenamer.h"

#include <editor/utils/assets.h>
#include <editor/context/EditorContext.h>

#include <y/io2/Buffer.h>

#include <imgui/yave_imgui.h>


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
		_root("", filesystem()->current_path().unwrap_or(core::String())) {

	set_current(&_root);
}

void ResourceBrowser::asset_selected(const FileInfo& file) {
	switch(file.type) {
		case AssetType::Material:
			if(const auto material = context()->loader().load<Material>(file.id)) {
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
	unused(file);
	return true;
}

void ResourceBrowser::set_path(std::string_view path) {
	if(!filesystem()->is_parent(_current->full_path, path).unwrap_or(true)) {
		set_current(nullptr);
	}
	bool it = true;
	while(it) {
		it = false;
		for(DirNode& node : _current->children) {
			if(node.full_path.view() == path || filesystem()->is_parent(node.full_path, path).unwrap_or(false)) {
				set_current(&node);
				it = true;
				break;
			}
		}
	}
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
	bool is_error = fs->for_each(node->full_path, [&](const auto& name) {
			const auto full_name = fs->join(node->full_path, name);
			if(fs->is_directory(full_name).unwrap_or(false)) {
				node->children.emplace_back(name, full_name, node);
			} else {
				node->files.emplace_back(context(), name, full_name);
			}
		}).is_error();

	if(is_error) {
		log_msg(fmt("Unable to explore \"%\"", node->full_path), Log::Warning);
	}

	node->up_to_date = true;
}

void ResourceBrowser::update_search() {
	_search_node = nullptr;
	if(const auto* searchable = dynamic_cast<const SearchableFileSystemModel*>(filesystem())) {
		std::string_view pattern = std::string_view(_search_pattern.data());
		std::string_view full_pattern = fmt("%%%%", _current->full_path, "%", pattern, "%");

		if(auto result = searchable->search(full_pattern)) {
			_search_node = std::make_unique<DirNode>(pattern, full_pattern);

			for(const core::String& full_name : result.unwrap()) {
				const core::String name = searchable->filename(full_name);
				if(searchable->is_directory(full_name).unwrap_or(false)) {
					_search_node->children.emplace_back(name, full_name, _search_node.get());
				} else {
					_search_node->files.emplace_back(context(), name, full_name);
				}
			}
		}
	}
}

void ResourceBrowser::finish_search() {
	_deferred << [=] { _search_node = nullptr; };
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
		const usize index = _current_hovered_index - _current->children.size();
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

void ResourceBrowser::make_drop_target(const DirNode* node) {
	if(ImGui::BeginDragDropTarget()) {
		if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("YAVE_ASSET_NAME")) {
			const char* original_name = reinterpret_cast<const char*>(payload->Data);
			const FileSystemModel* fs = filesystem();
			if(!context()->asset_store().rename(original_name, fs->join(node->full_path, fs->filename(original_name)))) {
				log_msg(fmt("Unable to move \"%\" to \"%\".", original_name, node->full_path), Log::Error);
			}
			refresh();
		}
		ImGui::EndDragDropTarget();
	}
}


// ----------------------------------- Context menu -----------------------------------

void ResourceBrowser::paint_context_menu() {
	if(ImGui::BeginPopup("###contextmenu")) {
		if(ImGui::Selectable("New folder")) {
			if(!filesystem()->create_directory(filesystem()->join(_current->full_path, "new folder"))) {
				log_msg("Unable to create directory.", Log::Error);
			}
			refresh();
		}

		if(const FileInfo* file = hovered_file()) {
			ImGui::Separator();
			if(ImGui::Selectable("Rename")) {
				context()->ui().add<AssetRenamer>(file->full_name);
			}
			if(ImGui::Selectable("Delete")) {
				if(!context()->asset_store().remove(file->id)) {
					log_msg("Unable to delete asset.", Log::Error);
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
					log_msg("Unable to delete folder.", Log::Error);
				}
				refresh();
			}
		}

		ImGui::Separator();
		if(ImGui::Selectable("Import objects")) {
			context()->ui().add<SceneImporter>(_current->full_path);
		}
		if(ImGui::Selectable("Import image")) {
			context()->ui().add<ImageImporter>(_current->full_path);
		}
		ImGui::Separator();
		if(ImGui::Selectable("Create material")) {
			const SimpleMaterialData material;
			io2::Buffer buffer;
			serde3::WritableArchive arc(buffer);
			if(arc.serialize(material)) {
				buffer.reset();
				AssetStore& store = context()->asset_store();
				if(!store.import(buffer, store.filesystem()->join(_current->full_path, "new material"), AssetType::Material)) {
					log_msg("Unable to import new material.", Log::Error);
				}
			} else {
				log_msg("Unable to create new material.", Log::Error);
			}

			refresh();
		}

		ImGui::EndPopup();
	}
}


// ----------------------------------- Tree view -----------------------------------

void ResourceBrowser::paint_tree_view(float width) {
	ImGui::BeginChild("###resourcestree", ImVec2(width, 0), true);
	draw_node(&_root, "store");
	ImGui::EndChild();
}

void ResourceBrowser::draw_node(DirNode* node, const core::String& name) {
	static constexpr ImGuiTreeNodeFlags default_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
	const ImGuiTreeNodeFlags flags = default_node_flags/* | (node->children.is_empty() ? ImGuiTreeNodeFlags_Leaf : 0)*/;

	// folding/expending the node will still register as a goto somehow...
	if(ImGui::TreeNodeEx(fmt(ICON_FA_FOLDER " %", name).data(), flags)) {
		make_drop_target(node);
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
		make_drop_target(node);
		if(ImGui::IsItemClicked(0)) {
			set_current(node);
		}
	}
}


// ----------------------------------- Asset list -----------------------------------

void ResourceBrowser::paint_asset_list(float width) {
	const bool menu_openned = ImGui::IsPopupOpen("###contextmenu");

	ImGui::BeginChild("###resources", ImVec2(width, 0), true);

	// back
	{
		if(_current->parent && ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
			set_current(_current->parent);
		}

		make_drop_target(_current->parent);

		if(!menu_openned && ImGui::IsItemHovered()) {
			reset_hover();
		}
	}

	usize index = 0;
	usize hovered = usize(-1);
	const auto selected = [&] { return _current_hovered_index == index; };

	// dirs
	const auto curr = _current;
	for(DirNode& n : curr->children) {
		if(ImGui::Selectable(fmt(ICON_FA_FOLDER " %", n.name).data(), selected())) {
			set_current(&n);
		}

		make_drop_target(&n);

		if(ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("YAVE_ASSET_NAME", n.full_path.data(), n.full_path.size() + 1);
			ImGui::EndDragDropSource();
		}

		if(!menu_openned && ImGui::IsItemHovered()) {
			hovered = index;
		}
		++index;
	}

	// files
	_preview_id = AssetId::invalid_id();
	const UiSettings& settings = context()->settings().ui();
	for(const FileInfo& file : curr->files) {
		if(!settings.filter_assets || display_asset(file)) {
			if(ImGui::Selectable(fmt("% %", asset_type_icon(file.type), file.name).data(), selected())) {
				asset_selected(file);
			}

			if(ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload("YAVE_ASSET_NAME", file.full_name.data(), file.full_name.size() + 1);
				ImGui::EndDragDropSource();
			}

			if(!menu_openned && ImGui::IsItemHovered()) {
				_preview_id = file.id;
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
		_refresh = false;
	}

	ImGui::EndChild();
}

void ResourceBrowser::paint_search_results(float width) {
	Y_TODO(Replace by table when available)
	ImGui::BeginChild("###searchresults", ImVec2(width, 0), true);
	{
		for(const DirNode& dir : _search_node->children) {
			if(ImGui::Selectable(fmt(ICON_FA_FOLDER " %", dir.full_path).data())) {
				set_path(dir.full_path);
				finish_search();
			}
		}

		_preview_id = AssetId::invalid_id();
		for(const FileInfo& file : _search_node->files) {
			if(ImGui::Selectable(fmt("% %", asset_type_icon(file.type), file.full_name).data())) {
				set_path(file.full_name);
				finish_search();
			}

			if(ImGui::IsItemHovered()) {
				_preview_id = file.id;
			}
		}
	}
	ImGui::EndChild();
}


// ----------------------------------- Preview -----------------------------------

void ResourceBrowser::paint_preview(float width) {
	if(_preview_id != AssetId::invalid_id()) {
		ImGui::BeginGroup();

		const auto thumb_data = context()->thumbmail_cache().get_thumbmail(_preview_id);
		if(TextureView* image = thumb_data.image) {
			ImGui::Image(image, math::Vec2(width));
		}
		ImGui::Text("ID: 0x%.8x", unsigned(_preview_id.id()));
		for(const auto& [name, value] : thumb_data.properties) {
			ImGui::TextUnformatted(fmt_c_str("%: %", name, value));
		}

		ImGui::EndGroup();
	}
}


// ----------------------------------- Header -----------------------------------

void ResourceBrowser::paint_header() {
	ImGui::PushID("###paintpath");

	if(ImGui::Button(ICON_FA_PLUS " Import")) {
		ImGui::OpenPopup("###importmenu");
	}

	if(ImGui::BeginPopup("###importmenu")) {
		if(ImGui::Selectable("Import objects")) {
			context()->ui().add<SceneImporter>(_current->full_path);
		}
		if(ImGui::Selectable("Import image")) {
			context()->ui().add<ImageImporter>(_current->full_path);
		}
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	{
		ImGui::PushStyleColor(ImGuiCol_Button, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

		paint_path_node(_current);

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	paint_search_bar();

	ImGui::PopID();
}

void ResourceBrowser::paint_search_bar() {
	bool has_seach_bar = false;
	if(dynamic_cast<const SearchableFileSystemModel*>(filesystem())) {
		const usize search_bar_size = 200;
		if(search_bar_size < size().x()) {
			has_seach_bar = true;
			ImGui::SameLine(size().x() - search_bar_size);
			if(ImGui::InputTextWithHint("###searchbar", " " ICON_FA_SEARCH, _search_pattern.data(), _search_pattern.size())) {
				update_search();
			}
			if(!_search_node && ImGui::IsItemClicked()) {
				update_search();
			}
		}
	}


	if(!has_seach_bar || !_search_pattern[0]) {
		finish_search();
	}
}

void ResourceBrowser::paint_path_node(DirNode* node) {
	if(!node) {
		return;
	}

	if(node->parent) {
		paint_path_node(node->parent);
		ImGui::SameLine();
	}

	ImGui::PushID(fmt_c_str("%", node));

	const char* name = node->name.is_empty() ? "/" : node->name.data();
	if(ImGui::Button(name)) {
		_deferred << [=] { set_current(node); };
	}

	ImGui::SameLine();

	std::string_view popup_name = "dirmenu";
	if(ImGui::Button(ICON_FA_ANGLE_RIGHT)) {
		Y_TODO(update node without blowing up _current)
		ImGui::OpenPopup(popup_name.data());
	}

	if(ImGui::BeginPopup(popup_name.data())) {
		for(DirNode& c : node->children) {
			if(ImGui::Selectable(fmt_c_str(ICON_FA_FOLDER " %", c.name))) {
				set_current(&c);
				break;
			}
		}
		ImGui::EndPopup();
	}

	ImGui::PopID();
}


// ----------------------------------- Main UI -----------------------------------

void ResourceBrowser::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	unused(recorder, token);
	y_profile();

	const bool show_tree = false;

	const float width = ImGui::GetWindowContentRegionWidth();
	const float tree_width = show_tree ? std::min(width * 0.5f, 200.0f) : 0.0f;
	const bool render_preview = width - tree_width > 400.0f;
	const float list_width = render_preview ? width - tree_width - 200.0f : 0.0f;
	const float preview_width = width - tree_width - list_width;

	paint_header();

	if(show_tree) {
		paint_tree_view(tree_width);
		ImGui::SameLine();
	}

	if(_search_node) {
		paint_search_results(list_width);
	} else {
		paint_asset_list(list_width);
	}


	if(render_preview) {
		ImGui::SameLine();
		paint_preview(preview_width);
	}

	{
		for(auto&& action : _deferred) {
			action();
		}
		_deferred.clear();
	}
}

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->asset_store().filesystem();
}

}
