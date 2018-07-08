/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include "AssetPtr.h"
#include "AssetStore.h"

#include <unordered_map>

namespace yave {

template<typename T>
struct AssetTraits {
	template<typename U>
	using has_load_from_t = typename U::load_from;

	static_assert(is_detected_v<has_load_from_t, T>, "Asset type should have ::load_from");

	// todo: try to deduce using TMP ?
	using load_from = typename T::load_from;
};

template<typename T>
class AssetLoader : public DeviceLinked {

	using traits = AssetTraits<T>;
	using load_from = typename traits::load_from;

	public:
		AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store) : DeviceLinked(dptr), _store(store) {
		}

		AssetStore& store() {
			return *_store;
		}


		AssetPtr<T> load(AssetId id) {
			Y_LOG_PERF("asset,loading");

			if(id == assets::invalid_id) {
				y_throw("Invalid id.");
			}

			auto& asset_ptr = _loaded[id];
			if(asset_ptr) {
				return asset_ptr;
			}

			auto asset = serde::deserialized<load_from>( _store->data(id));
			return asset_ptr = make_asset_with_id<T>(id, device(), std::move(asset));
		}

		AssetPtr<T> load(std::string_view name) noexcept {
			return load(_store->id(name));
		}

		AssetPtr<T> load_or_import(std::string_view name, std::string_view import_from, AssetStore::ImportType import_type) {
			AssetId id = assets::invalid_id;
			try {
				id = _store->id(name);
			} catch(...) {
				id = _store->import_as(import_from, name, import_type);
			}
			return load(id);
		}


	private:
		std::unordered_map<AssetId, AssetPtr<T>> _loaded;

		std::shared_ptr<AssetStore> _store;
};

}

#endif // YAVE_ASSETS_ASSETLOADER_H
