/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include <atomic>

namespace yave {

template<typename T>
class AssetPtr;
class GenericAssetPtr;

class AssetLoader;
class AssetLoadingContext;
class AssetLoadingThreadPool;


enum class AssetLoadingState : u32 {
    NotLoaded = 0,
    Loaded = 1,
    Failed = 2
};

enum class AssetLoadingErrorType : u32 {
    InvalidID,
    UnknownID,
    InvalidData,
    UnknownType,
    FailedDependency,
    Unknown
};

enum class AssetLoadingFlags : u32 {
    None = 0,
    SkipFailedDependenciesBit = 0x01
};

inline constexpr AssetLoadingFlags operator|(AssetLoadingFlags l, AssetLoadingFlags r) {
    return AssetLoadingFlags(u32(l) | u32(r));
}

inline constexpr AssetLoadingFlags operator&(AssetLoadingFlags l, AssetLoadingFlags r)  {
    return AssetLoadingFlags(u32(l) & u32(r));
}


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

        inline AssetLoader* loader() const;

    protected:
        inline AssetPtrDataBase(AssetId i, AssetLoader* loader, AssetLoadingState s = AssetLoadingState::NotLoaded);

        std::atomic<AssetLoadingState> _state = AssetLoadingState::NotLoaded;
        AssetLoader* _loader = nullptr;
};

template<typename T>
class AssetPtrData final : public AssetPtrDataBase {
    public:
        T asset;
        std::shared_ptr<AssetPtrData<T>> reloaded;

        inline AssetPtrData(AssetId id, AssetLoader* loader);
        inline AssetPtrData(AssetId id, AssetLoader* loader, T t);

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

        inline AssetPtr& operator=(std::nullptr_t);

        inline AssetId id() const;

        inline void wait_until_loaded() const;
        inline bool flush_reload();
        inline void reload();

        inline bool is_empty() const;
        inline bool has_loader() const;
        inline AssetLoader* loader() const;

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

        void load(AssetLoadingContext& context);

        y_reflect(_id)

    protected:
        friend class GenericAssetPtr;
        friend class AssetLoader;
        friend class AssetLoadingThreadPool;

        template<typename U, typename... Args>
        friend AssetPtr<U> make_asset(Args&&... args);

        template<typename U, typename... Args>
        friend AssetPtr<U> make_asset_with_id(AssetId id, Args&&... args);

        inline AssetPtr(AssetId id);
        inline AssetPtr(AssetId id, AssetLoader* loader);
        inline AssetPtr(AssetId id, AssetLoader* loader, T asset);
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
        friend class AssetLoader;
        friend class AssetLoadingThreadPool;

        std::shared_ptr<detail::AssetPtrDataBase> _data;
};




namespace detail {
template<typename T>
struct IsAssetPtr {
    static constexpr bool value = false;
};
template<typename T>
struct IsAssetPtr<AssetPtr<T>> {
    static constexpr bool value = true;
    using type = T;
};
}

template<typename T>
static constexpr bool is_asset_ptr_v = detail::IsAssetPtr<T>::value;


}

#include "AssetPtr.inl"

#endif // YAVE_ASSETS_ASSETPTR_H

