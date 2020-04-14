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
class AssetLoadingThreadPool;
class GenericAssetPtr;

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

class AssetPtrDataBase : NonMovable {
	public:
		const AssetId id;

		inline void set_failed(AssetLoadingErrorType error);
		inline AssetLoadingErrorType error() const;

		inline bool is_loaded() const;
		inline bool is_failed() const;
		inline bool is_loading() const;

	protected:
		inline AssetPtrDataBase(AssetId i, AssetLoadingState s = AssetLoadingState::NotLoaded);

		std::atomic<AssetLoadingState> _state = AssetLoadingState::NotLoaded;
};

template<typename T>
class AssetPtrData final : public AssetPtrDataBase {
	public:
		T asset;

		Y_TODO(Change to Loader<T>)
		LoaderBase* loader = nullptr;

		std::shared_ptr<AssetPtrData<T>> reloaded;

		inline AssetPtrData(AssetId i, LoaderBase* l);
		inline AssetPtrData(AssetId i, LoaderBase* l, T t);

		inline void finalize_loading(T t);
		inline void set_reloaded(const std::shared_ptr<AssetPtrData<T>>& other);
};

}


template<typename T>
class AssetPtr {
	protected:

	using Data = detail::AssetPtrData<T>;

	public:
		AssetPtr() = default;

		inline AssetId id() const;

		inline void wait_until_loaded() const;
		inline bool flush_reload();
		inline void reload();


		inline bool is_empty() const;
		inline bool has_loader() const;

		inline bool is_loaded() const;
		inline bool is_loading() const;
		inline bool is_failed() const;
		inline AssetLoadingErrorType error() const;

		inline core::Result<core::String> name() const;

		inline const T* get() const;

		inline const T& operator*() const;
		inline const T* operator->() const;
		inline explicit operator bool() const;

		inline bool operator==(const AssetPtr& other) const;
		inline bool operator!=(const AssetPtr& other) const;

		inline void post_deserialize(AssetLoadingContext& context);

		y_serde3(_id)

	protected:
		friend class AssetLoadingThreadPool;
		friend class GenericAssetPtr;
		friend class LoaderBase;
		friend class detail::AssetPtrData<T>;
		friend class Loader<T>;
		friend class AsyncAssetPtr<T>;

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset(Args&&... args);

		template<typename U, typename... Args>
		friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);

		inline AssetPtr(AssetId id);
		inline AssetPtr(AssetId id, LoaderBase* loader);
		inline AssetPtr(AssetId id, LoaderBase* loader, T asset);
		inline AssetPtr(std::shared_ptr<Data> ptr);

	protected:
		Y_TODO(Use intrusive smart ptr to save on space here)

		std::shared_ptr<Data> _data;

		AssetId _id;
};

class GenericAssetPtr {
	public:
		GenericAssetPtr() = default;

		template<typename T>
		GenericAssetPtr(const AssetPtr<T>& ptr) : _data(ptr._data, ptr._data ? ptr._data.get() : nullptr) {
		}

		bool is_empty() const {
			return !_data;
		}

		bool is_loaded() const {
			return is_empty() || _data->is_loaded();
		}

		bool is_loading() const {
			return !is_empty() && _data->is_loading();
		}

		bool is_failed() const {
			return !is_empty() && _data->is_failed();
		}

		AssetId id() const {
			return is_empty() ? AssetId::invalid_id() : _data->id;
		}

		AssetLoadingErrorType error() const {
			y_debug_assert(is_failed());
			return _data->error();
		}

	private:
		friend class AssetLoadingThreadPool;
		friend class LoaderBase;

		std::shared_ptr<detail::AssetPtrDataBase> _data;

};

}

#include "AssetPtr.inl"

#endif // YAVE_ASSETS_ASSETPTR_H
