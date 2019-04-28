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
#include <typeindex>

namespace yave {

class GenericAssetLoader : NonCopyable, public DeviceLinked {

		class LoaderBase : NonCopyable {
			public:
				virtual ~LoaderBase() {
				}
		};

		template<typename T>
		class Loader final : public LoaderBase {
			using traits = AssetTraits<T>;
			static_assert(traits::is_asset, "Type is missing asset traits");

			public:
				AssetPtr<T> load(const GenericAssetLoader& loader, AssetId id) {
					y_profile();

					if(id == AssetId::invalid_id()) {
						y_throw("Invalid id.");
					}

					auto& asset_ptr = _loaded[id];
					if(asset_ptr) {
						return asset_ptr;
					}

					io::ReaderRef data = loader.store().data(id);
					auto asset = serde::deserialized<typename traits::load_from>(data);
					return asset_ptr = make_asset_with_id<T>(id, loader.device(), std::move(asset));
				}

			private:
				std::unordered_map<AssetId, AssetPtr<T>> _loaded;
		};

	   public:
			GenericAssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store) : DeviceLinked(dptr), _store(store) {
			}

			AssetStore& store() {
				return *_store;
			}

			const AssetStore& store() const {
				return *_store;
			}

			template<typename T>
			AssetPtr<T> load(AssetId id) {
				return loader_for_type<T>().load(*this, id);
			}

			template<typename T>
			AssetPtr<T> load(std::string_view name) {
				return load<T>(store().id(name));
			}

			template<typename T>
			AssetPtr<T> import(std::string_view name, std::string_view import_from) {
				return load<T>(load_or_import(name, import_from));
			}

	   private:
			template<typename T>
			Loader<T>& loader_for_type() {
				 auto& loader = _loaders[typeid(T)];
				 if(!loader) {
					 loader = std::make_unique<Loader<T>>();
				 }
				 return *dynamic_cast<Loader<T>*>(loader.get());
			}


			AssetId load_or_import(std::string_view name, std::string_view import_from);

			std::shared_ptr<AssetStore> _store;
			std::unordered_map<std::type_index, std::unique_ptr<LoaderBase>> _loaders;
};

}

#endif // YAVE_ASSETS_ASSETLOADER_H
