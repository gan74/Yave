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

class AssetLoader : NonCopyable, public DeviceLinked {
	public:
		enum class ErrorType {
			InvalidID,
			UnknownID,
			InvalidData,
			Unknown
		};

		template<typename T>
		using Result = core::Result<AssetPtr<T>, ErrorType>;

	private:
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
				Result<T> load(AssetLoader& loader, AssetId id) noexcept {
					y_profile();

					if(id == AssetId::invalid_id()) {
						return core::Err(ErrorType::InvalidID);
					}

					auto& asset_ptr = _loaded[id];
					if(asset_ptr) {
						return core::Ok(asset_ptr);
					}

					if(auto reader = loader.store().data(id)) {
						if(auto asset = traits::load_asset(reader.unwrap(), loader)) {
							asset_ptr = make_asset_with_id<T>(id, std::move(asset.unwrap()));
							return core::Ok(asset_ptr);
						}
					}

					return core::Err(ErrorType::Unknown);
				}

			private:
				std::unordered_map<AssetId, AssetPtr<T>> _loaded;
		};

   public:
		AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store) : DeviceLinked(dptr), _store(store) {
		}

		AssetStore& store() {
			return *_store;
		}

		const AssetStore& store() const {
			return *_store;
		}

		template<typename T>
		Result<T> load(AssetId id) {
			return loader_for_type<T>().load(*this, id);
		}

		template<typename T>
		Result<T> load(std::string_view name) {
			return load<T>(store().id(name));
		}

		template<typename T>
		Result<T> import(std::string_view name, std::string_view import_from) {
			return load<T>(load_or_import(name, import_from));
		}

   private:
		template<typename T, typename E>
		Result<T> load(core::Result<AssetId, E> id) {
			if(id) {
				return load<T>(id.unwrap());
			}
			return core::Err(ErrorType::UnknownID);
		}

		template<typename T>
		Loader<T>& loader_for_type() {
			 auto& loader = _loaders[typeid(T)];
			 if(!loader) {
				 loader = std::make_unique<Loader<T>>();
			 }
			 return *dynamic_cast<Loader<T>*>(loader.get());
		}


		core::Result<AssetId> load_or_import(std::string_view name, std::string_view import_from);

		std::shared_ptr<AssetStore> _store;
		std::unordered_map<std::type_index, std::unique_ptr<LoaderBase>> _loaders;
};

}

#endif // YAVE_ASSETS_ASSETLOADER_H
