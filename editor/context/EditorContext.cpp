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

#include "EditorContext.h"
#include <yave/device/Device.h>
#include <yave/assets/FolderAssetStore.h>

#include <y/io2/File.h>

namespace editor {

DevicePtr ContextLinked::device() const {
	return _ctx ? _ctx->device() : nullptr;
}

EditorContext::EditorContext(DevicePtr dptr) :
		DeviceLinked(dptr),
		_resource_pool(std::make_shared<FrameGraphResourcePool>(device())),
		_asset_store(std::make_shared<FolderAssetStore>()),
		_loader(device(), _asset_store),
		_scene_view(&_default_scene_view),
		_ui(this),
		_thumb_cache(this),
		_picking_manager(this) {

	load_world();
}

EditorContext::~EditorContext() {
}

void EditorContext::flush_reload() {
	defer([this]() {
		y_profile_zone("flush reload");
		_selection.flush_reload();
		_thumb_cache.clear();
	});
}

void EditorContext::defer(core::Function<void()> func) {
	std::unique_lock _(_deferred_lock);
	if(_is_flushing_deferred) {
		y_fatal("Defer called from already deferred function.");
	}
	_deferred.emplace_back(std::move(func));
}

void EditorContext::flush_deferred() {
	y_profile();
	std::unique_lock lock(_deferred_lock);
	if(!_deferred.is_empty()) {
		device()->wait_all_queues();
		_is_flushing_deferred = true;
		for(auto& f : _deferred) {
			f();
		}
		_deferred.clear();
		_is_flushing_deferred = false;
	}
	_world.flush();
}

void EditorContext::set_scene_view(SceneView* scene) {
	if(!scene) {
		_scene_view = &_default_scene_view;
	} else {
		_scene_view = scene;
	}
}

void EditorContext::remove_scene_view(SceneView* scene) {
	if(_scene_view == scene) {
		set_scene_view(nullptr);
	}
}

SceneView& EditorContext::scene_view() {
	return *_scene_view;
}

SceneView& EditorContext::default_scene_view() {
	return _default_scene_view;
}

ecs::EntityWorld& EditorContext::world() {
	return _world;
}

const FileSystemModel* EditorContext::filesystem() const {
	return _filesystem.get() ? _filesystem.get() : FileSystemModel::local_filesystem();
}

const std::shared_ptr<FrameGraphResourcePool>& EditorContext::resource_pool() const {
	return _resource_pool;
}

Settings& EditorContext::settings() {
	return _setting;
}

Selection& EditorContext::selection() {
	return _selection;
}

AssetLoader& EditorContext::loader() {
	return _loader;
}

Ui& EditorContext::ui() {
	return _ui;
}

ThumbmailCache& EditorContext::thumbmail_cache() {
	return _thumb_cache;
}

PickingManager& EditorContext::picking_manager() {
	return _picking_manager;
}

AssetStore& EditorContext::asset_store() {
	return *_asset_store;
}

void EditorContext::save_world() const {
	auto file = io2::File::create("world.yw");
	if(!file) {
		log_msg("Unable to open file.", Log::Error);
		return;
	}

	WritableAssetArchive ar(file.unwrap());
	if(!_world.serialize(ar)) {
		log_msg("Unable to serialize world.", Log::Error);
	}
}

void EditorContext::load_world() {
	auto file = io2::File::open("world.yw");
	if(!file) {
		log_msg("Unable to open file.", Log::Error);
		return;
	}

	ecs::EntityWorld world;
	ReadableAssetArchive ar(file.unwrap(), _loader);
	if(!world.deserialize(ar)) {
		log_msg("Unable to deserialize world.", Log::Error);
		return;
	}

	_world = std::move(world);
}

void EditorContext::new_world() {
	_world = ecs::EntityWorld();
}

}
