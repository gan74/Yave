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

#include <y/core/Functor.h>
#include <y/serde3/archives.h>

#include <list>
#include <unordered_map>
#include <mutex>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

template<typename T>
class Loader final : public LoaderBase {

	using traits = AssetTraits<T>;
	static_assert(traits::is_asset, "Type is missing asset traits");
	using load_from_t = typename traits::load_from;

	public:
		using Result = core::Result<AssetPtr<T>, ErrorType>;

		Loader(AssetLoader* parent);

		Result load(AssetId id);
		Result reload(const AssetPtr<T>& ptr);

		AssetPtr<T> load_async(AssetId id);
		AssetPtr<T> reload_async(const AssetPtr<T>& ptr);

		/*std::pair<AssetPtr<T>, bool> create_empty(AssetId id);
		Result reload(AssetPtr<T> ptr);*/

		AssetType type() const;

	private:
		//AssetPtr<T> load_internal(AssetId id, bool async, bool reload);
		template<bool Reload>
		void load_internal(AssetPtr<T>& ptr, bool async);

		std::unordered_map<AssetId, WeakAssetPtr<T>> _loaded;
		std::recursive_mutex _lock;
};

}

#include "Loader.inl"

#endif // YAVE_ASSETS_LOADER_H
