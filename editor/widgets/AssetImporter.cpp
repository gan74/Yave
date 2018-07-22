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

#include "AssetImporter.h"

#include <editor/context/EditorContext.h>

#include <y/io/Buffer.h>

#include <imgui/imgui.h>

namespace editor {

static core::String clean_name(std::string_view name) {
	core::String str;
	str.set_min_capacity(name.size());
	for(char c : name) {
		str.push_back(std::isalnum(c) || c == '.' || c == '_' ? c : '_');
	}
	return str;
}

AssetImporter::AssetImporter(ContextPtr ctx) : Widget("Asset importer"), ContextLinked(ctx) {
	_browser.set_has_parent(true);
	_browser.set_extension_filter(import::supported_extensions());
	_browser.set_selected_callback([this](const auto& filename) { import_async(filename); return true; });
	_browser.set_canceled_callback([this] { close(); return true; });
}

void AssetImporter::paint_ui(CmdBufferRecorder<>& recorder, const FrameToken& token)  {
	_browser.paint(recorder, token);

	if(is_loading()) {
		if(done_loading()) {
			try {
				_imported = std::make_unique<import::SceneData>(_import_future.get());
				_browser.set_filesystem(context()->loader().asset_store().filesystem());
				_browser.set_flags(FileBrowser::SelectDirectory);
				_browser.set_selected_callback([this](const auto& dirname) { save_imports(dirname); return true; });
				_browser.show();
			} catch(std::exception& e) {
				context()->ui().ok("Unable to import", "Unable to import scene: "_s + e.what());
				_browser.show();
			}
		} else {
			ImGui::Text("Loading...");
		}
	}

}

bool AssetImporter::is_loading() const {
	return _import_future.valid();
}

bool AssetImporter::done_loading() const {
	return _import_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void AssetImporter::import_async(const core::String& filename) {
	_import_future = std::async(std::launch::async, [=] {
		//try {
			return import::import_scene(filename);
		/*} catch(...) {
			pr.set_exception(std::current_exception());
		}*/
	});
}

void AssetImporter::save_imports(const core::String& dirname) {
	auto& store = context()->loader().asset_store();
	{
		core::DebugTimer _("AssetImporter::save_imports meshes");
		for(const auto& mesh : _imported->meshes) {
			io::Buffer buffer;
			try {
				mesh.obj().serialize(buffer);
				store.import(buffer, store.filesystem()->join(dirname, clean_name(mesh.name())));
			} catch(std::exception& e) {
				log_msg("Unable to import mesh: "_s + e.what(), Log::Error);
			}
		}
	}

	{
		core::DebugTimer _("AssetImporter::save_imports animations");
		for(const auto& anim : _imported->animations) {
			io::Buffer buffer;
			try {
				anim.obj().serialize(buffer);
				store.import(buffer, store.filesystem()->join(dirname, clean_name(anim.name())));
			} catch(std::exception& e) {
				log_msg("Unable to import animation: "_s + e.what(), Log::Error);
			}
		}
	}

	close();
}

}
