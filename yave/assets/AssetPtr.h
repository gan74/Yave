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

#include "LoaderBase.h"

#include <memory>
#include <atomic>

namespace yave {

class AssetLoadingContext;

enum class AssetLoadingState : u32 {
	NotLoaded = 0,
	Loaded = 1,
	Failed = 2
};

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args);

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args);

template<typename T>
class AssetPtr {


	struct Data : NonMovable {
		T asset;
		const AssetId id;

		LoaderBase* loader = nullptr;

		std::shared_ptr<Data> reloaded;
		std::atomic<AssetLoadingState> state = AssetLoadingState::NotLoaded;

		Y_TODO(loader might be dangling)

		Data(AssetId i, LoaderBase* l);
		Data(AssetId i, LoaderBase* l, T t);

		void finalize_loading(T t);
		void set_reloaded(const AssetPtr<T>& other);

		void set_failed(AssetLoadingErrorType error);
		AssetLoadingErrorType error() const;

		bool is_loaded() const;
		bool is_failed() const;
	};

	public:
		AssetPtr() = default;

		const T* get() const;

		const T& operator*() const;
		const T* operator->() const;
		explicit operator bool() const;

		AssetId id() const;

		const T* flushed() const;
		void flush() const;
		bool flush_reload();
		void reload();
		void reload_async();

		bool is_empty() const;

		bool should_flush() const;
		bool has_loader() const;
		bool is_loaded() const;
		bool is_loading() const;
		bool is_failed() const;
		AssetLoadingErrorType error() const;

		bool operator==(const AssetPtr& other) const;
		bool operator!=(const AssetPtr& other) const;


		y_serde3(_id)
		void post_deserialize(const AssetLoadingContext& context);

	private:
		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset(Args&&... args);

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);

		friend class WeakAssetPtr<T>;
		friend class GenericAssetPtr;
		friend class LoaderBase;
		friend class Loader<T>;

		AssetPtr(AssetId id);
		AssetPtr(AssetId id, LoaderBase* loader);
		AssetPtr(AssetId id, LoaderBase* loader, T asset);
		AssetPtr(std::shared_ptr<Data> ptr);

	private:
		Y_TODO(Use intrusive smart ptr to save on space here)

		mutable T* _ptr = nullptr;
		std::shared_ptr<Data> _data;

		AssetId _id;
};

template<typename T>
class WeakAssetPtr {
	public:
		WeakAssetPtr() = default;
		WeakAssetPtr(const AssetPtr<T>& ptr);

		AssetPtr<T> lock() const;
		bool is_empty() const;

	private:
		std::weak_ptr<typename AssetPtr<T>::Data> _ptr;
};

}

#include "AssetPtr.inl"

#endif // YAVE_ASSETS_ASSETPTR_H
