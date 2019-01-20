/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef YAVE_ASSETS_ASSETLOADER_H
#define YAVE_ASSETS_ASSETLOADER_H

#include <yave/device/DeviceLinked.h>
#include <y/core/String.h>
#include <y/serde/serde.h>
#include <y/io/Buffer.h>

#include "AssetPtr.h"
#include "AssetStore.h"

#include <unordered_map>

namespace yave {

class AssetLoaderBase : NonCopyable, public DeviceLinked {
	public:
		AssetLoaderBase(DevicePtr dptr, const std::shared_ptr<AssetStore>& store);

		AssetStore& store();

	protected:
		AssetId load_or_import(std::string_view name, std::string_view import_from);

	private:
		std::shared_ptr<AssetStore> _store;
};

template<typename T>
class AssetLoader : public AssetLoaderBase {

	using traits = AssetTraits<T>;
	using load_from = typename traits::load_from;

	public:
		AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store) : AssetLoaderBase(dptr, store) {
		}

		AssetPtr<T> load(AssetId id) {
			Y_LOG_PERF("asset,loading");

			if(id == AssetId::invalid_id()) {
				y_throw("Invalid id.");
			}

			auto& asset_ptr = _loaded[id];
			if(asset_ptr) {
				return asset_ptr;
			}

			auto asset = serde::deserialized<load_from>(store().data(id));
			return asset_ptr = make_asset_with_id<T>(id, device(), std::move(asset));
		}

		AssetPtr<T> load(std::string_view name) {
			return load(store().id(name));
		}

		AssetPtr<T> import(std::string_view name, std::string_view import_from) {
			return load(load_or_import(name, import_from));
		}

	private:
		std::unordered_map<AssetId, AssetPtr<T>> _loaded;

};

}

#endif // YAVE_ASSETS_ASSETLOADER_H
