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
#ifndef YAVE_ASSETS_ASSETPTR_H
#define YAVE_ASSETS_ASSETPTR_H

#include "AssetLoaderBase.h"

#include <memory>
#include <atomic>

namespace yave {

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
	struct Data : NonCopyable {
		const T asset;
		const AssetId id;

		AssetLoaderBase* loader = nullptr;
		std::shared_ptr<Data> reloaded;

		Y_TODO(loader might be dangling)

		template<typename... Args>
		Data(AssetId i, AssetLoaderBase* l, Args&&... args) : asset(y_fwd(args)...), id(i), loader(l) {
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
			if(is_reloaded()) {
				_ptr = std::atomic_load(&_ptr->reloaded);
				y_debug_assert(_ptr->id == _id);
				flush_reload(); // in case the we reloaded several time
				return true;
			}
			return false;
		}

		void reload() {
			if(_ptr && _ptr->loader) {
				_ptr->loader->reload(_ptr->id);
				flush_reload();
			}
		}

		bool operator==(const AssetPtr& other) const {
			return _ptr == other._ptr;
		}

		bool operator!=(const AssetPtr& other) const {
			return _ptr != other._ptr;
		}

		// AssetLoader.h
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
		explicit AssetPtr(AssetId id, AssetLoaderBase* loader, Args&&... args) :
				_ptr(std::make_shared<Data>(id, loader, y_fwd(args)...)),
				_id(id) {
		}

		AssetPtr(std::shared_ptr<Data> ptr) : _ptr(std::move(ptr)) {
			if(_ptr) {
				_id = _ptr->id;
			}
		}

	private:
		std::shared_ptr<Data> _ptr;

		Y_TODO(Use id stored in _ptr?)
		AssetId _id;
};

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args) {
	return AssetPtr<T>(AssetId::invalid_id(), nullptr, y_fwd(args)...);
}

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args) {
	return AssetPtr<T>(id, nullptr, y_fwd(args)...);
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

		bool is_empty() const {
			return _ptr.expired();
		}

	private:
		std::weak_ptr<typename AssetPtr<T>::Data> _ptr;
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
		std::shared_ptr<const AssetId> _id;
};

}

#endif // YAVE_ASSETS_ASSETPTR_H
