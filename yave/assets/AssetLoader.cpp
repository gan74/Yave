/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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


AssetLoader::LoaderBase::~LoaderBase() {
}

AssetLoader::LoaderBase::LoaderBase(AssetLoader* parent) : _parent(parent) {
}

AssetLoader* AssetLoader::LoaderBase::parent() const {
    return _parent;
}


AssetLoader::AssetLoader(const std::shared_ptr<AssetStore>& store, AssetLoadingFlags flags, usize concurency) :
        _store(store),
        _thread_pool(this, concurency),
        _loading_flags(flags) {
}

AssetLoader::~AssetLoader() {
}

AssetStore& AssetLoader::store() {
    return *_store;
}

const AssetStore& AssetLoader::store() const {
    return *_store;
}

void AssetLoader::set_loading_flags(AssetLoadingFlags flags) {
    _loading_flags = flags;
}

AssetLoadingFlags AssetLoader::loading_flags() const {
    return _loading_flags;
}

void AssetLoader::wait_until_loaded(const GenericAssetPtr& ptr) {
    _thread_pool.wait_until_loaded(ptr);
    y_debug_assert(!ptr.is_loading());
}

bool AssetLoader::is_loading() const {
    return _thread_pool.is_processing();
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

