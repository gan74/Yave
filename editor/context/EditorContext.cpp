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

namespace editor {

DevicePtr ContextLinked::device() const {
	return _ctx ? _ctx->device() : nullptr;
}

EditorContext::EditorContext(DevicePtr dptr) :
		DeviceLinked(dptr),
		_resource_pool(std::make_shared<FrameGraphResourcePool>(device())),
		_asset_store(std::make_shared<FolderAssetStore>()),
		_loader(device(), _asset_store),
		_scene(this),
		_ui(this),
		_thumb_cache(this),
		_picking_manager(this) {
}

EditorContext::~EditorContext() {
}

void EditorContext::defer(core::Function<void()>&& func) {
	std::unique_lock _(_deferred_lock);
	if(_is_flushing_deferred) {
		y_fatal("Defer called from already deferred function.");
	}
	_deferred.emplace_back(std::move(func));
}

void EditorContext::flush_deferred() {
	y_profile();
	std::unique_lock lcok(_deferred_lock);
	if(!_deferred.is_empty()) {
		device()->graphic_queue().wait();
		_is_flushing_deferred = true;
		for(auto& f : _deferred) {
			f();
		}
		_deferred.clear();
		_is_flushing_deferred = false;
	}
	_scene.flush();
}


}
