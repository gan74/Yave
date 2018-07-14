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

#include "SceneData.h"
#include "EditorContext.h"

#include <y/io/File.h>
#include <imgui/imgui.h>

namespace editor {

SceneData::SceneData(ContextPtr ctx) : ContextLinked(ctx), _scene_view(_scene) {
	load("scene.ys");
}

Scene& SceneData::scene() {
	return _scene;
}

SceneView& SceneData::view() {
	return _scene_view;
}

bool SceneData::is_scene_empty() const {
	return _scene.renderables().is_empty() &&
		   _scene.static_meshes().is_empty() &&
		   _scene.lights().is_empty();
}

math::Vec3 SceneData::to_screen_pos(const math::Vec3& world) {
	auto h_pos = _scene_view.camera().viewproj_matrix() * math::Vec4(world, 1.0f);

	return math::Vec3((h_pos.to<2>() / h_pos.w()) * 0.5f + 0.5f, h_pos.z() / h_pos.w());
}

math::Vec2 SceneData::to_window_pos(const math::Vec3& world) {
	math::Vec2 viewport = ImGui::GetWindowSize();
	math::Vec2 offset = ImGui::GetWindowPos();

	auto screen = to_screen_pos(world);

	if(screen.z() < 0.0f) {
		(std::fabs(screen.x()) > std::fabs(screen.y()) ? screen.x() : screen.y()) /= 0.0f; // infs
	}

	return screen.to<2>() * viewport + offset;
}

void SceneData::save(std::string_view filename) {
	Y_LOG_PERF("editor,save");
	if(auto r = io::File::create(filename); r.is_ok()) {
		try {
			_scene.serialize(r.unwrap());
		} catch(std::exception& e) {
			log_msg("Unable to save scene: "_s + e.what(), Log::Error);
		}
	} else {
		log_msg("Unable to create scene file.", Log::Error);
	}
}

void SceneData::load(std::string_view filename) {
	Y_LOG_PERF("editor,loading");
	if(auto r = io::File::open(filename); r.is_ok()) {
		try {
			auto sce = Scene::deserialized(r.unwrap(), context()->loader().static_mesh());
			_scene = std::move(sce);
		} catch(std::exception& e) {
			log_msg("Unable to load scene: "_s + e.what(), Log::Error);
		}
	} else {
		log_msg("Unable to open scene file.", Log::Error);
	}
}

void SceneData::flush() {
}


}
