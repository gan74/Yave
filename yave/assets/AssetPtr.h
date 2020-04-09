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


namespace detail {
template<typename T>
struct AssetPtrData : NonMovable {
	T asset;
	const AssetId id;

	Y_TODO(Change to Loader<T>)
	LoaderBase* loader = nullptr;

	std::shared_ptr<AssetPtrData<T>> reloaded;
	std::atomic<AssetLoadingState> state = AssetLoadingState::NotLoaded;

	AssetPtrData(AssetId i, LoaderBase* l);
	AssetPtrData(AssetId i, LoaderBase* l, T t);

	void finalize_loading(T t);
	void set_reloaded(const std::shared_ptr<AssetPtrData<T>>& other);

	void set_failed(AssetLoadingErrorType error);
	AssetLoadingErrorType error() const;

	bool is_loaded() const;
	bool is_failed() const;
};
}

template<typename T>
class AssetPtrBase {
	protected:

	using Data = detail::AssetPtrData<T>;

	public:
		AssetPtrBase() = default;

		AssetId id() const;

		bool flush_reload();
		void reload();

		bool is_empty() const;

		bool has_loader() const;
		bool is_loading() const;
		bool is_failed() const;
		AssetLoadingErrorType error() const;

		y_serde3(_id)

	protected:
		friend class detail::AssetPtrData<T>;
		friend class LoaderBase;
		friend class Loader<T>;
		friend class AsyncAssetPtr<T>;

		AssetPtrBase(AssetId id);
		AssetPtrBase(std::shared_ptr<Data> ptr);

	protected:
		Y_TODO(Use intrusive smart ptr to save on space here)

		std::shared_ptr<Data> _data;

		AssetId _id;
};

template<typename T>
class AssetPtr final : public AssetPtrBase<T> {

	using Data = typename AssetPtrBase<T>::Data;

	public:
		static constexpr bool is_async = false;

		AssetPtr() = default;

		const T* get() const;

		const T& operator*() const;
		const T* operator->() const;
		explicit operator bool() const;

		bool operator==(const AssetPtr& other) const;
		bool operator!=(const AssetPtr& other) const;

		void post_deserialize(const AssetLoadingContext& context);

	private:
		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset(Args&&... args);

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);

		friend class Loader<T>;

		AssetPtr(AssetId id);
		AssetPtr(AssetId id, LoaderBase* loader, T asset);
		AssetPtr(std::shared_ptr<Data> ptr);
};


template<typename T>
class AsyncAssetPtr final : public AssetPtrBase<T> {

	using Data = typename AssetPtrBase<T>::Data;

	public:
		static constexpr bool is_async = true;

		AsyncAssetPtr() = default;
		AsyncAssetPtr(const AssetPtr<T>& other);

		const T* get() const;

		const T& operator*() const;
		const T* operator->() const;
		explicit operator bool() const;

		bool is_loaded() const;
		bool should_flush() const;

		const T* flushed() const;
		void flush() const;

		bool operator==(const AsyncAssetPtr& other) const;
		bool operator!=(const AsyncAssetPtr& other) const;

		void post_deserialize(const AssetLoadingContext& context);

	private:
		friend class Loader<T>;

		AsyncAssetPtr(AssetId id);
		AsyncAssetPtr(AssetId id, LoaderBase* loader);
		AsyncAssetPtr(AssetId id, LoaderBase* loader, T asset);
		AsyncAssetPtr(std::shared_ptr<Data> ptr);

	private:
		mutable T* _ptr = nullptr;
};

/*template<typename T>
class WeakAssetPtr {

	using Data = detail::AssetPtrData<T>;

	public:
		WeakAssetPtr() = default;
		WeakAssetPtr(const AssetPtr<T>& ptr);

		AssetPtr<T> lock() const;
		bool is_expired() const;

	private:
		std::weak_ptr<Data> _data;
};*/

}

#include "AssetPtr.inl"

#endif // YAVE_ASSETS_ASSETPTR_H
