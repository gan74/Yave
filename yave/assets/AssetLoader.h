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

#include "Loader.h"
#include "AssetLoadingContext.h"
#include "AssetLoadingThreadPool.h"

#include <typeindex>
#include <future>

namespace yave {

class AssetLoadingContext;

class AssetLoader : NonMovable, public DeviceLinked {

   public:
		using ErrorType = AssetLoadingErrorType;

		template<typename T>
		using Result = core::Result<AssetPtr<T>, ErrorType>;

		AssetLoader(DevicePtr dptr, const std::shared_ptr<AssetStore>& store, usize concurency = 1);
		~AssetLoader();

		AssetStore& store();
		const AssetStore& store() const;

		// This is dangerous: Do not call in loading threads!
		void wait_until_loaded(const GenericAssetPtr& ptr);


		template<typename T>
		inline Result<T> load_res(AssetId id);
		template<typename T>
		inline Result<T> load_res(std::string_view name);

		template<typename T>
		inline AssetPtr<T> load(AssetId id);
		template<typename T>
		inline AssetPtr<T> load_async(AssetId id);


		template<typename T>
		inline Result<T> import(std::string_view name, std::string_view import_from);

   private:
		template<typename T>
		friend class Loader;

		friend class AssetLoadingContext;

		template<typename T, typename E>
		inline Result<T> load(core::Result<AssetId, E> id);

		template<typename T>
		inline Loader<T>& loader_for_type();

		core::Result<AssetId> load_or_import(std::string_view name, std::string_view import_from, AssetType type);

		std::unordered_map<std::type_index, std::unique_ptr<LoaderBase>> _loaders;
		std::shared_ptr<AssetStore> _store;

		std::recursive_mutex _lock;
		AssetLoadingThreadPool _thread_pool;
};

}


#include "AssetLoader.inl"

#endif // YAVE_ASSETS_ASSETLOADER_H
