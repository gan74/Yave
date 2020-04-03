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

#include "AssetLoader.h"

#include <y/io2/File.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

AssetLoader::AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store) : DeviceLinked(dptr), _store(store) {
}

AssetLoader::~AssetLoader() {
	if(const usize pending = _thread.pending_tasks()) {
		log_msg(fmt("% loading pending.", pending), Log::Warning);
	}
}

AssetStore& AssetLoader::store() {
	return *_store;
}

const AssetStore& AssetLoader::store() const {
	return *_store;
}

core::Result<AssetId> AssetLoader::load_or_import(std::string_view name, std::string_view import_from, AssetType type) {
	if(auto id = _store->id(name)) {
		return id;
	}

	if(auto file = io2::File::open(import_from)) {
		return _store->import(file.unwrap(), name, type);
	}

	return core::Err();
}

}
