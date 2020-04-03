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
#ifndef YAVE_ASSETS_ASSETLOADER_H
#define YAVE_ASSETS_ASSETLOADER_H

#include <yave/device/DeviceLinked.h>
#include <yave/utils/serde.h>

#include "AssetPtr.h"
#include "AssetStore.h"

#include <y/concurrent/StaticThreadPool.h>
#include <y/serde3/archives.h>

#include <y/utils/log.h>

#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <future>

namespace yave {

class AssetLoader : NonMovable, public DeviceLinked {
	public:
		enum class ErrorType {
			InvalidID,
			UnknownID,
			InvalidData,
			UnknownType,
			Unknown
		};

		template<typename T>
		using Result = core::Result<AssetPtr<T>, ErrorType>;

	private:
		template<typename T>
		class Loader final : public AssetLoaderBase {

			using traits = AssetTraits<T>;
			static_assert(traits::is_asset, "Type is missing asset traits");

			public:
				Loader(AssetLoader* parent) : AssetLoaderBase(parent) {
				}

				Result<T> load(AssetId id) {
					y_profile();
					if(id == AssetId::invalid_id()) {
						return core::Err(ErrorType::InvalidID);
					}

					auto [ptr, is_empty] = create_empty(id);
					if(!is_empty) {
						return core::Ok(std::move(ptr));
					}
					return reload(ptr);
				}

				std::pair<AssetPtr<T>, bool> create_empty(AssetId id) {
					const auto lock = y_profile_unique_lock(_lock);
					if(auto it = _loaded.find(id); it != _loaded.end()) {
						if(auto asset = it->second.lock()) {
							return {std::move(asset), false};
						}
					}
					AssetPtr<T> empty = AssetPtr<T>(id, this);
					_loaded[id] = empty;
					return {std::move(empty), true};
				}

				Result<T> reload(AssetPtr<T> ptr) {
					y_profile();
					if(!ptr._ptr || ptr._ptr->loader != this) {
						y_fatal("Invalid AssetPtr");
					}
					const AssetId id = ptr._ptr->id;
					if(auto reader = store().data(id)) {
						y_profile_zone("loading");
						serde3::ReadableArchive arc(std::move(reader.unwrap()));
						typename traits::load_from load_from;
						if(!arc.deserialize(load_from, *parent())) {
							return core::Err(ErrorType::InvalidData);
						}

						AssetPtr<T> reloaded = AssetPtr<T>(id, this, device(), std::move(load_from));
						std::atomic_store(&ptr._ptr->reloaded, reloaded._ptr);

						const auto lock = y_profile_unique_lock(_lock);
						_loaded[id] = reloaded;
						return core::Ok(std::move(reloaded));
					}
					return core::Err(ErrorType::Unknown);
				}


				void reload(AssetId id) override {
					if(id != AssetId::invalid_id()) {
						AssetPtr<T> loaded = [&] {
							const auto lock = y_profile_unique_lock(_lock);
							return _loaded[id].lock();
						}();
						if(loaded) {
							reload(loaded).ignore();
						}
					}
				}

				AssetType type() const override {
					return traits::type;
				}

				DevicePtr device() const {
					return parent()->device();
				}

				AssetStore& store() {
					return parent()->store();
				}

				const AssetStore& store() const {
					return parent()->store();
				}

			private:
				std::unordered_map<AssetId, WeakAssetPtr<T>> _loaded;

				Y_TODO(we load durring loading which might be bad)
				std::mutex _lock;
		};

   public:
		AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store);
		~AssetLoader();

		AssetStore& store();
		const AssetStore& store() const;

		/*template<typename T>
		AssetPtr<T> load_async_2(AssetId id) {
			auto& loader = loader_for_type<T>();
			AssetPtr<T> ptr = loader.create_empty(*this, id);
			return _thread.schedule([this, id] { return load<T>(id); });
			return ptr;
		}*/


		template<typename T>
		std::future<Result<T>> load_async(AssetId id) {
			return _thread.schedule_with_future([this, id]() -> Result<T> { return load<T>(id); });
		}

		template<typename T>
		Result<T> load(AssetId id) {
			return loader_for_type<T>().load(id);
		}

		template<typename T>
		Result<T> load(std::string_view name) {
			return load<T>(store().id(name));
		}

		template<typename T>
		Result<T> import(std::string_view name, std::string_view import_from) {
			return load<T>(load_or_import(name, import_from, AssetTraits<T>::type));
		}

		template<typename T>
		Result<T> set(AssetId, T) {
			log_msg("AssetLoader::set is not supported", Log::Error);
			return core::Err(ErrorType::Unknown);
		}

   private:
		template<typename T, typename E>
		Result<T> load(core::Result<AssetId, E> id) {
			if(id) {
				return load<T>(id.unwrap());
			}
			return core::Err(ErrorType::UnknownID);
		}

		AssetLoaderBase* loader_for_id(AssetId id) {
			if(const auto type = _store->asset_type(id)) {
				const auto lock = y_profile_unique_lock(_lock);
				for(const auto& [index, loader] : _loaders) {
					unused(index);
					if(loader->type() == type.unwrap()) {
						return loader.get();
					}
				}
			}
			return nullptr;
		}

		template<typename T>
		auto& loader_for_type() {
			const auto lock = y_profile_unique_lock(_lock);
			using Type = remove_cvref_t<T>;
			auto& loader = _loaders[typeid(Type)];
			if(!loader) {
				loader = std::make_unique<Loader<Type>>(this);
			}
			return *dynamic_cast<Loader<Type>*>(loader.get());
		}



		core::Result<AssetId> load_or_import(std::string_view name, std::string_view import_from, AssetType type);

		std::unordered_map<std::type_index, std::unique_ptr<AssetLoaderBase>> _loaders;
		std::shared_ptr<AssetStore> _store;

		std::mutex _lock;

		concurrent::WorkerThread _thread = concurrent::WorkerThread("Asset loading thread");
};

template<typename T>
void AssetPtr<T>::post_deserialize(AssetLoader& loader)  {
	if(auto r = loader.load<T>(_id)) {
		*this = r.unwrap();
	}
}

}

#endif // YAVE_ASSETS_ASSETLOADER_H
