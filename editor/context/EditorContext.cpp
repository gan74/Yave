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

#include "EditorContext.h"

#include "scenes.h"
#include "images.h"


#include <yave/assets/FolderAssetStore.h>
#include <yave/serialize/serialize.h>

#include <y/io/File.h>

#include <imgui/imgui.h>

namespace editor {

DevicePtr ContextLinked::device() const {
	return _ctx ? _ctx->device() : nullptr;
}

EditorContext::EditorContext(DevicePtr dptr) :
		DeviceLinked(dptr),
		asset_store(new FolderAssetStore()),
		texture_loader(dptr, asset_store),
		mesh_loader(dptr, asset_store),
		_scene(std::make_unique<Scene>()),
		_scene_view(std::make_unique<SceneView>(*_scene)) {


	_icon_textures << Texture(device(), images::light());
	_icon_textures << Texture(device(), images::save());
	_icon_textures << Texture(device(), images::load());

	_icons = Icons{_icon_textures[0], _icon_textures[1], _icon_textures[2]};

	load_settings();
	load_scene("./scene.ys");
}

EditorContext::~EditorContext() {
	save_settings();
}

void EditorContext::set_scene_deferred(Scene&& scene) {
	defer([=, sce{std::move(scene)}]() mutable {
		set_selected(nullptr);
		*_scene = std::move(sce);
	});
}

void EditorContext::save_scene(std::string_view filename) {
	Y_LOG_PERF("editor,save");
	if(auto r = io::File::create(filename); r.is_ok()) {
		if(_scene->to_file(r.unwrap()).is_error()) {
			log_msg("Unable to save scene.", Log::Error);
		}
	} else {
		log_msg("Unable to create scene file.", Log::Error);
	}
}

void EditorContext::load_scene(std::string_view filename) {
	Y_LOG_PERF("editor,loading");
	if(auto r = io::File::open(filename); r.is_ok()) {
		if(auto s = Scene::from_file(r.unwrap(), mesh_loader); s.is_ok()) {
			set_scene_deferred(std::move(s.unwrap()));
		} else {
			log_msg("Unable to load scene.", Log::Error);
		}
	} else {
		log_msg("Unable to open scene file.", Log::Error);
	}
}

void EditorContext::save_settings() {
	if(auto r = io::File::create("./editor_settings.dat"); r.is_ok()) {
		r.unwrap().write_one(camera_settings);
	}
}

void EditorContext::load_settings() {
	if(auto r = io::File::open("./editor_settings.dat"); r.is_ok()) {
		r.unwrap().read_one(camera_settings);
	}
}

Scene* EditorContext::scene() const {
	return _scene.get();
}

SceneView* EditorContext::scene_view() const {
	return _scene_view.get();
}

bool EditorContext::is_scene_empty() const {
	return _scene->renderables().is_empty() &&
		   _scene->static_meshes().is_empty() &&
		   _scene->lights().is_empty();
}

void EditorContext::set_selected(Light* sel) {
	_selected = sel;
	_selected_light = sel;
}

void EditorContext::set_selected(Transformable* sel) {
	_selected = sel;
	_selected_light = nullptr;
}

void EditorContext::set_selected(std::nullptr_t) {
	_selected = nullptr;
	_selected_light = nullptr;
}

Transformable* EditorContext::selected() const {
	return _selected;
}

Light* EditorContext::selected_light() const {
	return _selected_light;
}

void EditorContext::defer(core::Function<void()>&& func) {
	std::unique_lock _(_deferred_lock);
	if(_is_flushing_deferred) {
		y_fatal("Defer called from already deferred function.");
	}
	_deferred.emplace_back(std::move(func));
}

void EditorContext::flush_deferred() {
	std::unique_lock _(_deferred_lock);
	if(!_deferred.is_empty()) {
		Y_LOG_PERF("editor");
		device()->queue(QueueFamily::Graphics).wait();
		_is_flushing_deferred = true;
		for(auto& f : _deferred) {
			f();
		}
		_deferred.clear();
		_is_flushing_deferred = false;
	}
}


EditorContext::Icons* EditorContext::icons() const {
	return &_icons;
}

math::Vec3 EditorContext::to_screen_pos(const math::Vec3& world) {
	auto h_pos = _scene_view->camera().viewproj_matrix() * math::Vec4(world, 1.0f);

	return math::Vec3((h_pos.to<2>() / h_pos.w()) * 0.5f + 0.5f, h_pos.z() / h_pos.w());
}

math::Vec2 EditorContext::to_window_pos(const math::Vec3& world) {
	math::Vec2 viewport = ImGui::GetWindowSize();
	math::Vec2 offset = ImGui::GetWindowPos();

	auto screen = to_screen_pos(world);

	if(screen.z() < 0.0f) {
		(std::fabs(screen.x()) > std::fabs(screen.y()) ? screen.x() : screen.y()) /= 0.0f; // infs
	}

	return screen.to<2>() * viewport + offset;
}


}
