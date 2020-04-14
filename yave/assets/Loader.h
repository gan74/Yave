/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef YAVE_ASSETS_LOADER_H
#define YAVE_ASSETS_LOADER_H

#include "AssetPtr.h"
#include "AssetStore.h"
#include "AssetDependencies.h"

#include <y/core/Functor.h>
#include <y/core/Result.h>
#include <y/serde3/archives.h>

#include <unordered_map>
#include <mutex>

namespace yave {

template<typename T>
class Loader final : public LoaderBase {
	using Data = typename AssetPtr<T>::Data;
	using WeakAssetPtr = std::weak_ptr<Data>;

	using traits = AssetTraits<T>;
	static_assert(traits::is_asset, "Type is missing asset traits");

	using LoadFrom = typename traits::load_from;

	using CreateFunc = core::Function<void()>;
	using ReadFunc = core::Function<CreateFunc(AssetLoadingContext&)>;


	public:
		Loader(AssetLoader* parent);
		~Loader();

		inline AssetPtr<T> load(AssetId id);
		inline AssetPtr<T> load_async(AssetId id);

		inline AssetPtr<T> reload(const AssetPtr<T>& ptr);

		AssetType type() const {
			return traits::type;
		}

	private:
		[[nodiscard]] inline bool find_ptr(AssetPtr<T>& ptr);
		inline ReadFunc read_func(const std::shared_ptr<Data>& data);

		std::unordered_map<AssetId, WeakAssetPtr> _loaded;
		std::recursive_mutex _lock;
};

}

#include "Loader.inl"

#endif // YAVE_ASSETS_LOADER_H
