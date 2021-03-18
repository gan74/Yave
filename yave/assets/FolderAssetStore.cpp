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

#include "FolderAssetStore.h"

#include <y/io2/File.h>

#include <y/utils/log.h>
#include <y/serde3/archives.h>

#include <charconv>
#include <cinttypes>
#include <cstdio>

namespace yave {

static bool is_delimiter(char c) {
    return c == '/';
}

static std::string_view strict_path(std::string_view path) {
    const bool has_delim = !path.empty() && is_delimiter(path.back());
    const std::string_view no_delim(path.data(), path.size() - has_delim);
    return no_delim;
}

static std::string_view strict_parent_path(std::string_view path) {
    for(usize i = path.size(); i > 0; --i) {
        if(is_delimiter(path[i - 1])) {
            return path.substr(0, i - 1);
        }
    }
    return std::string_view();
}

static bool is_strict_direct_parent(std::string_view parent, std::string_view path) {
    return strict_parent_path(path) == parent;
}

static bool is_strict_indirect_parent(std::string_view parent, std::string_view path) {
    parent = strict_path(parent);
    if(parent.size() >= path.size()) {
        return false;
    }
    if(!is_delimiter(path[parent.size()])) {
        return false;
    }
    return path.substr(0, parent.size()) == parent;
}

static bool is_valid_name_char(char c) {
    return std::isalnum(c) || c == '_' || c == ' ';
}

static bool is_valid_path_char(char c) {
    return is_valid_name_char(c) || is_delimiter(c);
}

[[maybe_unused]]
static bool is_valid_name(std::string_view name) {
    for(char c : name) {
        if(!is_valid_name_char(c)) {
            return false;
        }
    }
    return !name.empty();
}

static bool is_valid_path(std::string_view name) {
    for(char c : name) {
        if(!is_valid_path_char(c)) {
            return false;
        }
    }
    return true;
}




FolderAssetStore::FolderFileSystemModel::FolderFileSystemModel(FolderAssetStore* parent) : _parent(parent) {
    y_debug_assert(core::String("a") < core::String("ab"));
    y_debug_assert(std::string_view("a") < std::string_view("ab"));
}

core::String FolderAssetStore::FolderFileSystemModel::join(std::string_view path, std::string_view name) const {
    if(!path.size()) {
        return name;
    }
    char last = path.back();
    core::String result;
    result.set_min_capacity(path.size() + name.size() + 1);
    result += path;
    if(!is_delimiter(last)) {
        result.push_back('/');
    }
    result += name;
    return result;
}

core::String FolderAssetStore::FolderFileSystemModel::filename(std::string_view path) const {
    for(usize i = path.size(); i > 0; --i) {
        if(is_delimiter(path[i - 1])) {
            return path.substr(i);
        }
    }
    return path;
}

FileSystemModel::Result<core::String> FolderAssetStore::FolderFileSystemModel::current_path() const {
    return core::Ok(core::String());
}

FileSystemModel::Result<core::String> FolderAssetStore::FolderFileSystemModel::parent_path(std::string_view path) const {
    return core::Ok(core::String(strict_parent_path(path)));
}

FileSystemModel::Result<bool> FolderAssetStore::FolderFileSystemModel::exists(std::string_view path) const {
    y_profile();

    if(path.empty()) {
        return core::Ok(true);
    }

    const bool has_delim = !path.empty() && is_delimiter(path.back());
    const std::string_view no_delim(path.data(), path.size() - has_delim);

    const auto lock = y_profile_unique_lock(_parent->_lock);
    return core::Ok(_parent->_folders.find(no_delim) != _parent->_folders.end() || (!has_delim && _parent->_assets.find(no_delim) != _parent->_assets.end()));
}

FileSystemModel::Result<bool> FolderAssetStore::FolderFileSystemModel::is_directory(std::string_view path) const {
    y_profile();

    if(path.empty()) {
        return core::Ok(true);
    }

    const auto lock = y_profile_unique_lock(_parent->_lock);
    return core::Ok(_parent->_folders.find(strict_path(path)) != _parent->_folders.end());
}

FileSystemModel::Result<core::String> FolderAssetStore::FolderFileSystemModel::absolute(std::string_view path) const {
    return core::Ok(core::String(path));
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
    y_profile();

    path = strict_path(path);

    const bool is_root = path.empty();

    const auto lock = y_profile_unique_lock(_parent->_lock);
    {
        for(auto it = _parent->_folders.lower_bound(path); it != _parent->_folders.end(); ++it) {
            if(is_strict_direct_parent(path, *it)) {
                func(it->sub_str(path.size() + !is_root));
            } else if(!it->starts_with(path)) {
                break;
            }
        }

        for(auto it = _parent->_assets.lower_bound(path); it != _parent->_assets.end(); ++it) {
            if(is_strict_direct_parent(path, it->first)) {
                func(it->first.sub_str(path.size() + !is_root));
            } else if(!it->first.starts_with(path)) {
                break;
            }
        }
    }

    //_parent->save().unwrap();

    return core::Ok();

}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::create_directory(std::string_view path) const {
    y_profile();

    path = strict_path(path);

    if(path.empty()) {
        return core::Ok();
    }

    const auto lock = y_profile_unique_lock(_parent->_lock);

    const auto parent = strict_parent_path(path);
    if(!is_directory(parent).unwrap_or(false)) {
        y_try(create_directory(parent));
    }

    if(_parent->_folders.emplace(path).second) {
        log_msg(fmt("Folder created: %", path));
        return _parent->save_or_restore();
    }

    return core::Ok();
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::remove(std::string_view path) const {
    y_profile();

    path = strict_path(path);

    const auto lock = y_profile_unique_lock(_parent->_lock);
    {
        for(auto it = _parent->_folders.lower_bound(path); it != _parent->_folders.end(); ++it) {
            if(is_strict_indirect_parent(path, *it) || std::string_view(*it) == path) {
                it = _parent->_folders.erase(it);
                --it;
            } else {
                y_debug_assert(!it->starts_with(path));
                break;
            }
        }

        for(auto it = _parent->_assets.lower_bound(path); it != _parent->_assets.end(); ++it) {
            if(is_strict_indirect_parent(path, it->first)) {
                it = _parent->_assets.erase(it);
                --it;
            } else {
                y_debug_assert(!it->first.starts_with(path));
                break;
            }
        }
    }

    return _parent->save_or_restore();
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::rename(std::string_view from, std::string_view to) const {
    y_profile();

    from = strict_path(from);
    to = strict_path(to);

    if(!is_valid_path(to) || to.empty() || from.empty()) {
        return core::Err();
    }

    const auto lock = y_profile_unique_lock(_parent->_lock);
    auto restore = [this]() -> Result<> {
        _parent->load().unwrap();
        return core::Err();
    };

    for(auto it = _parent->_folders.lower_bound(from); it != _parent->_folders.end(); ++it) {
        bool rename_folder = false;
        core::String new_name;

        if(is_strict_indirect_parent(from, *it)) {
            const std::string_view end = it->sub_str(from.size() + 1);
            new_name = join(to, end);
            rename_folder = true;
            y_debug_assert(!end.empty());
        } else if(from == *it) {
            new_name = to;
            rename_folder = true;
        }

        if(rename_folder) {
            y_debug_assert(!new_name.is_empty());
            if(!_parent->_folders.insert(std::move(new_name)).second) {
                return restore();
            }

            it = _parent->_folders.erase(it);
            --it;
        }

        Y_TODO(End condition)
    }

    for(auto it = _parent->_assets.lower_bound(from); it != _parent->_assets.end(); ++it) {

        if(is_strict_indirect_parent(from, it->first)) {
            const std::string_view end = it->first.sub_str(from.size() + 1);
            core::String new_name = join(to, end);
            y_debug_assert(!end.empty());
            y_debug_assert(!new_name.is_empty());

            if(!_parent->_assets.emplace(std::move(new_name), it->second).second) {
                return restore();
            }

            it = _parent->_assets.erase(it);
            --it;
        }

        Y_TODO(End condition)
    }

    return _parent->save_or_restore();
}








FolderAssetStore::FolderAssetStore(const core::String& root) : _root(FileSystemModel::local_filesystem()->absolute(root).unwrap_or(root)), _filesystem(this) {
    FileSystemModel::local_filesystem()->create_directory(_root).unwrap();
    if(!load()) {
        log_msg("Unable to load asset database.", Log::Error);
    }
}

FolderAssetStore::~FolderAssetStore() {
    // This should be done automatically
    // save().unwrap();
}

core::String FolderAssetStore::asset_file_name(AssetId id) const {
    std::array<char, 17> buffer = {0};
    std::snprintf(buffer.data(), buffer.size(), "%016" PRIx64, id.id());
    const std::string_view name(buffer.data(), buffer.size() - 1);
    return _filesystem.join(_root, name);
}

void FolderAssetStore::rebuild_id_map() const {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    if(!_ids) {
        _ids = std::make_unique<core::ExternalHashMap<AssetId, std::map<core::String, AssetData>::const_iterator>>();
        _ids->reserve(_assets.size());

        for(auto it = _assets.begin(); it != _assets.end(); ++it) {
            (*_ids)[it->second.id] = it;
        }
    }
}

const FileSystemModel* FolderAssetStore::filesystem() const {
    return &_filesystem;
}

AssetStore::Result<AssetId> FolderAssetStore::import(io2::Reader& data, std::string_view dst_name, AssetType type) {
    y_profile();

    dst_name = strict_path(dst_name);

    if(!is_valid_path(dst_name)) {
        return core::Err(ErrorType::InvalidName);
    }

    const auto lock = y_profile_unique_lock(_lock);

    if(!_filesystem.create_directory(strict_parent_path(dst_name))) {
        return core::Err(ErrorType::FilesytemError);
    }

    const AssetId id = next_id();
    const core::String filename = asset_file_name(id);

    if(!io2::File::copy(data, filename)) {
        return core::Err(ErrorType::FilesytemError);
    }

    _assets[dst_name] = AssetData{id, type};

    y_try(save_or_restore());
    return core::Ok(id);
}

AssetStore::Result<> FolderAssetStore::write(AssetId id, io2::Reader& data) {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    const core::String filename = asset_file_name(id);
    if(!io2::File::open(filename)) {
        return core::Err(ErrorType::UnknownID);
    }

    if(!io2::File::copy(data, filename)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<io2::ReaderPtr> FolderAssetStore::data(AssetId id) const {
    y_profile();

    if(auto file = io2::File::open(asset_file_name(id))) {
        io2::ReaderPtr ptr = std::make_unique<io2::File>(std::move(file.unwrap()));
        return core::Ok(std::move(ptr));
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<AssetId> FolderAssetStore::id(std::string_view name) const {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    if(const auto it = _assets.find(name); it != _assets.end()) {
        return core::Ok(it->second.id);
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<core::String> FolderAssetStore::name(AssetId id) const {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    rebuild_id_map();
    if(const auto it = _ids->find(id); it != _ids->end()) {
        return core::Ok(core::String(it->second->first));
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<> FolderAssetStore::remove(AssetId id) {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    auto na = name(id);
    y_try(na);

    if(!_filesystem.remove(na.unwrap())) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<> FolderAssetStore::rename(AssetId id, std::string_view new_name) {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    auto na = name(id);
    y_try(na);

    return rename(na.unwrap(), new_name);
}

AssetStore::Result<> FolderAssetStore::remove(std::string_view name) {
    y_profile();

    if(!_filesystem.remove(name)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<> FolderAssetStore::rename(std::string_view from, std::string_view to) {
    y_profile();

    if(!_filesystem.rename(from, to)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<AssetType> FolderAssetStore::asset_type(AssetId id) const {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    rebuild_id_map();
    if(const auto it = _ids->find(id); it != _ids->end()) {
        return core::Ok(it->second->second.type);
    }

    return core::Err(ErrorType::UnknownID);
}


AssetId FolderAssetStore::next_id() {
    y_profile();

    return AssetId::from_id(_next_id++);
}






core::String FolderAssetStore::index_file_name() const {
    const auto* fs = FileSystemModel::local_filesystem();
    return fs->join(_root, ".index");
}

FolderAssetStore::Result<> FolderAssetStore::load() {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    _folders.clear();
    _assets.clear();
    _ids = nullptr;

    core::Vector<u8> data;
    if(auto file = io2::File::open(index_file_name()); file.is_error() || file.unwrap().read_all(data).is_error()) {
        return core::Err(ErrorType::FilesytemError);
    }

    core::String line;
    usize index = 0;


    // Folders
    {
        for(; index != data.size(); ++index) {
            const char c = data[index];
            if(c == '\n') {
                if(line.is_empty()) {
                    break;
                }
                y_debug_assert(is_valid_path(line));
                _folders.insert(std::move(line));
                line.make_empty();
            } else {
                line.push_back(c);
            }
        }
    }

    ++index;

    // Assets
    {
        for(; index != data.size(); ++index) {
            const char c = data[index];
            if(c == '\n') {
                if(line.is_empty()) {
                    break;
                }
                AssetId asset_id;
                AssetType asset_type = AssetType::Unknown;

                const char* beg = line.begin();
                {
                    u64 id = asset_id.id();
                    const auto res = std::from_chars(beg, line.end(), id);
                    if(res.ec == std::errc::invalid_argument || res.ptr == line.end()) {
                        return core::Err(ErrorType::Unknown);
                    }
                    beg = res.ptr + 1;
                    asset_id = AssetId::from_id(id);
                }

                {
                    u32 type = u32(asset_type);
                    const auto res = std::from_chars(beg, line.end(), type);
                    if(res.ec == std::errc::invalid_argument || res.ptr == line.end()) {
                        return core::Err(ErrorType::Unknown);
                    }
                    beg = res.ptr + 1;
                    asset_type = AssetType(type);
                }

                _assets[core::String(beg)] = AssetData{asset_id, asset_type};
                line.make_empty();
            } else {
                line.push_back(c);
            }
        }
    }

    ++index;

    // ID
    {
        for(; index != data.size(); ++index) {
            const char c = data[index];
            if(c == '\n') {
                u64 id = 0;
                if(std::from_chars(line.begin(), line.end(), id).ec == std::errc::invalid_argument) {
                    return core::Err(ErrorType::Unknown);
                }
                _next_id = id;
                break;
            } else {
                line.push_back(c);
            }
        }
    }

    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::save() {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);

    _ids = nullptr;

    core::String data;
    {
        for(const core::String& folder : _folders) {
            y_debug_assert(is_valid_path(folder));
            y_debug_assert(std::string_view(folder) == strict_path(folder));
            y_debug_assert(_filesystem.is_directory(strict_parent_path(folder)).unwrap_or(false));

            //fmt_into(data, "%\n", folder);
            data += folder;
            data += "\n";
        }

        data += "\n";

        for(const auto& asset : _assets) {
            y_debug_assert(is_valid_path(asset.first));
            y_debug_assert(std::string_view(asset.first) == strict_path(asset.first));
            y_debug_assert(_filesystem.is_directory(strict_parent_path(asset.first)).unwrap_or(false));

            //fmt_into(data, "% % %\n", asset.second.id, usize(asset.second.type), asset.first);
            data += fmt("% % ", asset.second.id.id(), usize(asset.second.type));
            data += asset.first;
            data += "\n";
        }

        data += fmt("\n%\n", _next_id);
    }

    {
        y_profile_zone("writing");

        const core::String index_file = index_file_name();
        const core::String tmp_file = index_file + "_";

        Y_TODO(Openning file is slow, maybe we should cache it)
        if(auto file = io2::File::create(tmp_file); file.is_error() || file.unwrap().write_array(data.data(), data.size()).is_error()) {
            return core::Err(ErrorType::FilesytemError);
        }

        y_profile_zone("renaming");
        if(!FileSystemModel::local_filesystem()->rename(tmp_file, index_file)) {
            return core::Err(ErrorType::FilesytemError);
        }
    }

    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::save_or_restore() {
    const auto lock = y_profile_unique_lock(_lock);
    if(!save()) {
        load().unwrap();
        log_msg("Failed to save", Log::Error);
        return core::Err(ErrorType::FilesytemError);
    }
    return core::Ok();
}

}

