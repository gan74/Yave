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
#ifndef YAVE_ASSETS_ASSETPTR_H
#define YAVE_ASSETS_ASSETPTR_H

#include "AssetId.h"

#include <memory>

namespace yave {

class AssetLoader;
class GenericAssetPtr;

template<typename T>
class AssetPtr;

template<typename T>
class WeakAssetPtr;


template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args);

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args);


template<typename T>
class AssetPtr {
	struct Pair : NonCopyable {
		const T asset;
		const AssetId id;
		AssetPtr<T> reloaded;

		template<typename... Args>
		Pair(AssetId i, Args&&... args) : asset(y_fwd(args)...), id(i) {
		}
	};

	public:
		AssetPtr() = default;

		const T* get() const {
			return &_ptr->asset;
		}

		const T& operator*() const {
			return _ptr->asset;
		}

		const T* operator->() const {
			return &_ptr->asset;
		}

		explicit operator bool() const {
			return bool(_ptr);
		}

		AssetId id() const {
			return _id;
		}

		bool is_reloaded() const {
			return _ptr && _ptr->reloaded;
		}

		bool flush_reload() {
			if(_ptr) {
				if(const auto ptr = _ptr->reloaded) {
					y_debug_assert(ptr.id() == id());
					_ptr = std::move(ptr._ptr);
					return true;
				}
			}
			return false;
		}

		bool operator==(const AssetPtr& other) const {
			return _ptr == other._ptr;
		}

		bool operator!=(const AssetPtr& other) const {
			return _ptr != other._ptr;
		}


		// AssetLoader.h
		serde2::Result serialize(WritableAssetArchive& arc) const noexcept;
		serde2::Result deserialize(ReadableAssetArchive& arc) noexcept;

		y_serde3(_id)
		void post_deserialize(AssetLoader& loader);

	private:
		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset(Args&&... args);

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);

		friend class WeakAssetPtr<T>;
		friend class GenericAssetPtr;
		friend class AssetLoader;

		template<typename... Args>
		explicit AssetPtr(AssetId id, Args&&... args) :
				_ptr(std::make_shared<Pair>(id, y_fwd(args)...)),
				_id(id) {
		}

		AssetPtr(std::shared_ptr<Pair> ptr) : _ptr(std::move(ptr)) {
			if(_ptr) {
				_id = _ptr->id;
			}
		}

	private:
		std::shared_ptr<Pair> _ptr;
		AssetId _id;
};

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args) {
	return AssetPtr<T>(AssetId::invalid_id(), y_fwd(args)...);
}

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args) {
	return AssetPtr<T>(id, y_fwd(args)...);
}


template<typename T>
class WeakAssetPtr {
	public:
		WeakAssetPtr() = default;

		WeakAssetPtr(const AssetPtr<T>& ptr) : _ptr(ptr._ptr) {
		}

		AssetPtr<T> lock() const {
			return _ptr.lock();
		}

	private:
		std::weak_ptr<typename AssetPtr<T>::Pair> _ptr;
};


class GenericAssetPtr {

	public:
		GenericAssetPtr() = default;

		template<typename T>
		GenericAssetPtr(const AssetPtr<T>& ptr) : _id(ptr._ptr, &ptr._ptr->id) {
		}

		AssetId id() const {
			return _id ? *_id : AssetId::invalid_id();
		}

	private:
		std::shared_ptr<AssetId> _id;
};

}

#endif // YAVE_ASSETS_ASSETPTR_H
