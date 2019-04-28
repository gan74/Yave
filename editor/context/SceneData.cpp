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

#include "SceneData.h"
#include "EditorContext.h"

#include <yave/material/Material.h>

#include <y/io/File.h>

#include <imgui/imgui.h>

namespace editor {

SceneData::SceneData(ContextPtr ctx) :
		ContextLinked(ctx),
		_default_scene_view(_scene),
		_scene_view(&_default_scene_view) {

	load("scene.ys");
}


void SceneData::set_scene_view(SceneView* scene) {
	if(!scene) {
		_scene_view = &_default_scene_view;
	} else {
		_scene_view = scene;
	}
}

void SceneData::reset_scene_view(SceneView* scene) {
	if(_scene_view == scene) {
		set_scene_view(nullptr);
	}
}

Scene& SceneData::scene() {
	return _scene;
}

SceneView& SceneData::scene_view() {
	return *_scene_view;
}

SceneView& SceneData::default_scene_view() {
	return _default_scene_view;
}

bool SceneData::is_scene_empty() const {
	return _scene.renderables().is_empty() &&
		   _scene.static_meshes().is_empty() &&
		   _scene.lights().is_empty();
}

void SceneData::save(std::string_view filename) {
	y_profile();
	try {
		_scene.serialize(io::File::create(filename).or_throw("Unable to create scene file."));
	} catch(std::exception& e) {
		log_msg(fmt("Unable to save scene: %", e.what()), Log::Error);
	}
}

void SceneData::load(std::string_view filename) {
	y_profile();
	try {
		auto default_mat = make_asset<Material>(device()->default_resources()[DefaultResources::BasicMaterial]);
		auto sce = Scene::deserialized(
				io::File::open(filename).or_throw("Unable to open scene file."),
				context()->loader());
		_scene = std::move(sce);
	} catch(std::exception& e) {
		log_msg(fmt("Unable to load scene: %", e.what()), Log::Error);
	}
}

StaticMeshInstance* SceneData::add(AssetId id) {
	AssetPtr<StaticMesh> mesh = context()->loader().load<StaticMesh>(id);
	auto material = make_asset<Material>(device()->default_resources()[DefaultResources::BasicMaterial]);
	auto inst = std::make_unique<StaticMeshInstance>(mesh, material);
	StaticMeshInstance* inst_ptr = inst.get();
	_to_add << std::move(inst);
	return inst_ptr;
}


StaticMeshInstance* SceneData::add(std::string_view name) {
	return add(context()->asset_store().id(name));
}

void SceneData::flush() {
	_scene.static_meshes().emplace_back(_to_add.begin(), _to_add.end());
	_to_add.clear();
}


}
