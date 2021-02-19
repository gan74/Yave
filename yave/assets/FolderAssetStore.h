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
#ifndef YAVE_ASSETS_FOLDERASSETSTORE_H
#define YAVE_ASSETS_FOLDERASSETSTORE_H

#include <yave/utils/FileSystemModel.h>

#include "AssetStore.h"

#include <y/core/String.h>
#include <y/core/HashMap.h>

#include <mutex>
#include <set>
#include <map>

namespace yave {

class FolderAssetStore final : NonMovable, public AssetStore {

    class FolderFileSystemModel final : public FileSystemModel {
        public:

            core::String filename(std::string_view path) const override;
            core::String  join(std::string_view path, std::string_view name) const override;

            Result<core::String> current_path() const override;
            Result<core::String> parent_path(std::string_view path) const override;

            Result<bool> exists(std::string_view path) const override;
            Result<bool> is_directory(std::string_view path) const override;

            Result<core::String> absolute(std::string_view path) const override;
            Result<> for_each(std::string_view path, const for_each_f& func) const override;
            Result<> create_directory(std::string_view path) const override;
            Result<> remove(std::string_view path) const override;
            Result<> rename(std::string_view from, std::string_view to) const override;

            //Result<core::Vector<core::String>> search(std::string_view pattern) const override;

        private:
            friend class FolderAssetStore;

            FolderFileSystemModel(FolderAssetStore* parent);

            FolderAssetStore* _parent = nullptr;
    };

    struct AssetData {
        AssetId id;
        AssetType type;
    };

    public:
        FolderAssetStore(const core::String& root = "./store");
        ~FolderAssetStore() override;

        const FileSystemModel* filesystem() const override;

        Result<AssetId> import(io2::Reader& data, std::string_view dst_name, AssetType type) override;
        Result<> write(AssetId id, io2::Reader& data) override;

        Result<AssetId> id(std::string_view name) const override;
        Result<core::String> name(AssetId id) const override;

        Result<io2::ReaderPtr> data(AssetId id) const override;

        Result<> remove(AssetId id) override;
        Result<> rename(AssetId id, std::string_view new_name) override;
        Result<> remove(std::string_view name) override;
        Result<> rename(std::string_view from, std::string_view to) override;

        Result<AssetType> asset_type(AssetId id) const override;

    private:
        core::String index_file_name() const;
        core::String asset_file_name(AssetId id) const;
        AssetId next_id();
        void rebuild_id_map() const;

        Result<> load();
        Result<> save();
        Result<> save_or_restore();

        core::String _root;

        std::atomic<u64> _next_id = 0;
        std::set<core::String> _folders;
        std::map<core::String, AssetData> _assets;

        mutable std::unique_ptr<core::ExternalHashMap<AssetId, std::map<core::String, AssetData>::const_iterator>> _ids;

        mutable std::recursive_mutex _lock;

        FolderFileSystemModel _filesystem;

};
}


#endif // YAVE_ASSETS_FOLDERASSETSTORE_H

