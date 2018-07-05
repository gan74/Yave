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
#ifndef YAVE_ASSETS_ASSETPTR_H
#define YAVE_ASSETS_ASSETPTR_H

#include <yave/yave.h>
#include <memory>

namespace yave {

using AssetId = i64;

namespace assets {
static constexpr AssetId invalid_id = std::numeric_limits<AssetId>::lowest();
}

template<typename T>
class AssetPtr;

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args);

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args);


template<typename T>
class AssetPtr {
	struct Pair : NonCopyable {
		const T asset;
		AssetId id;

		template<typename... Args>
		Pair(AssetId i, Args&&... args) : asset(std::forward<Args>(args)...), id(i) {
		}
	};

	public:
		AssetPtr() = default;

		const T* get() const noexcept {
			return &_ptr->asset;
		}

		const T& operator*() const noexcept {
			return _ptr->asset;
		}

		const T* operator->() const noexcept {
			return &_ptr->asset;
		}

		explicit operator bool() const noexcept {
			return bool(_ptr);
		}

		AssetId id() const noexcept {
			return _ptr ? _ptr->id : assets::invalid_id;
		}

	private:
		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset(Args&&... args);

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);


		template<typename... Args>
		explicit AssetPtr(AssetId id, Args&&... args) : _ptr(std::make_shared<Pair>(id, std::forward<Args>(args)...)) {
		}

	private:
		std::shared_ptr<Pair> _ptr;
};

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args) {
	return AssetPtr<T>(assets::invalid_id, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args) {
	return AssetPtr<T>(id, std::forward<Args>(args)...);
}

}

#endif // YAVE_ASSETS_ASSETPTR_H
