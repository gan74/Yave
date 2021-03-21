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
#ifndef YAVE_ASSETS_ASSETLOADER_H
#define YAVE_ASSETS_ASSETLOADER_H

#include <y/core/HashMap.h>

#include <yave/graphics/graphics.h>

#include "AssetStore.h"
#include "AssetLoadingContext.h"
#include "AssetLoadingThreadPool.h"

#include <typeindex>
#include <future>

namespace yave {

class AssetLoadingContext;

class AssetLoader : NonMovable {
    public:
         using ErrorType = AssetLoadingErrorType;

         template<typename T>
         using Result = core::Result<AssetPtr<T>, ErrorType>;

    private:
        using LoadingJob = AssetLoadingThreadPool::LoadingJob;

        class LoaderBase : NonMovable {
            public:
                virtual ~LoaderBase();

                AssetLoader* parent() const;

                virtual AssetType type() const = 0;

            protected:
                LoaderBase(AssetLoader* parent);

            private:

                AssetLoader* _parent = nullptr;
        };


        template<typename T>
        class Loader final : public LoaderBase {
            using Data = typename AssetPtr<T>::Data;
            using WeakAssetPtr = std::weak_ptr<Data>;

            static_assert(sizeof(T) > 0, "Type is not defined");

            using traits = AssetTraits<T>;
            static_assert(traits::is_asset, "Type is missing asset traits");

            using LoadFrom = typename traits::load_from;

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
                inline std::unique_ptr<LoadingJob> create_loading_job(AssetPtr<T> ptr);

                core::ExternalHashMap<AssetId, WeakAssetPtr> _loaded;
                std::recursive_mutex _lock;
        };

   public:
        Y_TODO(make configurable)
        static constexpr bool fail_on_partial_deser = false;

        AssetLoader(const std::shared_ptr<AssetStore>& store, AssetLoadingFlags flags = AssetLoadingFlags::None, usize concurency = 1);
        ~AssetLoader();

        AssetStore& store();
        const AssetStore& store() const;

        void set_loading_flags(AssetLoadingFlags flags);
        AssetLoadingFlags loading_flags() const;

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
        inline AssetPtr<T> reload(const AssetPtr<T>& ptr);

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

        core::ExternalHashMap<std::type_index, std::unique_ptr<LoaderBase>> _loaders;
        std::shared_ptr<AssetStore> _store;

        std::recursive_mutex _lock;
        AssetLoadingThreadPool _thread_pool;

        std::atomic<AssetLoadingFlags> _loading_flags = AssetLoadingFlags::None;
};

}


#include "AssetLoader.inl"

#endif // YAVE_ASSETS_ASSETLOADER_H

