/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include <y/concurrent/StaticThreadPool.h>

#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/serde3/archives.h>

#include <charconv>

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
    return std::isprint(static_cast<unsigned char>(c)) && c != '\\';
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

    const auto lock = std::unique_lock(_parent->_lock);
    return core::Ok(_parent->_folders.find(no_delim) != _parent->_folders.end() || (!has_delim && _parent->_assets.find(no_delim) != _parent->_assets.end()));
}

FileSystemModel::Result<FileSystemModel::EntryType> FolderAssetStore::FolderFileSystemModel::entry_type(std::string_view path) const {
    y_profile();

    if(path.empty()) {
        return core::Ok(EntryType::Directory);
    }

    const auto lock = std::unique_lock(_parent->_lock);
    const bool is_dir = _parent->_folders.find(strict_path(path)) != _parent->_folders.end();
    return core::Ok(is_dir ? EntryType::Directory : EntryType::File);
}

FileSystemModel::Result<core::String> FolderAssetStore::FolderFileSystemModel::absolute(std::string_view path) const {
    return core::Ok(core::String(path));
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
    y_profile();

    path = strict_path(path);

    const bool is_root = path.empty();

    const auto lock = std::unique_lock(_parent->_lock);

    for(auto it = _parent->_folders.lower_bound(path); it != _parent->_folders.end(); ++it) {
        if(is_strict_direct_parent(path, *it)) {
            const EntryInfo info = {
                EntryType::Directory,
                it->sub_str(path.size() + !is_root),
                0
            };
            func(info);
        } else if(!it->starts_with(path)) {
            break;
        }
    }

    for(auto it = _parent->_assets.lower_bound(path); it != _parent->_assets.end(); ++it) {
        if(is_strict_direct_parent(path, it->first)) {
            const EntryInfo info = {
                EntryType::File,
                it->first.sub_str(path.size() + !is_root),
                it->second.file_size
            };
            func(info);
        } else if(!it->first.starts_with(path)) {
            break;
        }
    }

    return core::Ok();

}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::create_directory(std::string_view path) const {
    y_profile();

    path = strict_path(path);

    if(path.empty()) {
        return core::Ok();
    }

    const auto lock = std::unique_lock(_parent->_lock);

    const auto parent = strict_parent_path(path);
    if(!is_directory(parent).unwrap_or(false)) {
        y_try(create_directory(parent));
    }

    if(_parent->_folders.emplace(path).second) {
        log_msg(fmt("Folder created: {}", path));
        return _parent->save_or_restore_tree();
    }

    return core::Ok();
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::remove(std::string_view path) const {
    y_profile();

    path = strict_path(path);

    const auto lock = std::unique_lock(_parent->_lock);

    core::Vector<core::String> files_to_delete;

    std::set<core::String> new_folders;
    {
        y_profile_zone("folders");
        for(const core::String& folder : _parent->_folders) {
            if(is_strict_indirect_parent(path, folder) || folder == path) {
                // Remove
            } else {
                y_debug_assert(!is_strict_direct_parent(path, folder));
                new_folders.insert(folder);
            }
        }
    }

    std::map<core::String, AssetData> new_assets;
    {
        y_profile_zone("assets");
        for(const auto &[name, data] : _parent->_assets) {
            if(is_strict_indirect_parent(path, name) || name == path) {
                const AssetId id = data.id;
                const core::String filename = _parent->asset_desc_file_name(id);
                if(auto r = FileSystemModel::local_filesystem()->remove(filename); !r) {
                    log_msg(fmt("Unable to remove {}", filename), Log::Error);
                    _parent->reload_all().ignore();
                    return r;
                }
                files_to_delete << _parent->asset_data_file_name(id);
            } else {
                y_debug_assert(!is_strict_direct_parent(path, name));
                new_assets[name] = data;
            }
        }
    }

    if(!files_to_delete.is_empty()) {
        y_profile_zone("cleaning files");
        for(const core::String& file : files_to_delete) {
            FileSystemModel::local_filesystem()->remove(file).ignore();
        }
    }

    log_msg(fmt("Removed {} assets", _parent->_assets.size() - new_assets.size()));

    _parent->_ids = nullptr;
    std::swap(new_folders, _parent->_folders);
    std::swap(new_assets, _parent->_assets);

    return _parent->save_or_restore_tree();
}

FileSystemModel::Result<> FolderAssetStore::FolderFileSystemModel::rename(std::string_view from, std::string_view to) const {
    y_profile();

    from = strict_path(from);
    to = strict_path(to);

    if(!is_valid_path(to) || to.empty() || from.empty()) {
        return core::Err();
    }

    const auto lock = std::unique_lock(_parent->_lock);

    std::map<core::String, AssetData> new_assets;
    {
        for(const auto &[name, data] : _parent->_assets) {
            if(is_strict_indirect_parent(from, name) || from == name) {
                const std::string_view end = name.sub_str(from.size() + 1);
                const core::String new_name = end.empty() ? core::String(to) : join(to, end);

                if(_parent->_assets.emplace(new_name, data).second) {
                    if(auto r = _parent->load_desc(data.id)) {
                        AssetDesc desc = std::move(r.unwrap());
                        desc.name = new_name;
                        if(_parent->save_desc(data.id, desc)) {
                            new_assets[new_name] = data;
                            continue;
                        }
                    }
                }

                _parent->reload_all().ignore();
                return core::Err();
            } else {
                y_debug_assert(!is_strict_direct_parent(from, name));
                new_assets[name] = data;
            }
        }
    }

    std::set<core::String> new_folders;
    {
        for(const core::String& folder : _parent->_folders) {
            core::String new_name = folder;

            if(is_strict_indirect_parent(from, folder)) {
                const std::string_view end = folder.sub_str(from.size() + 1);
                new_name = join(to, end);
                y_debug_assert(!end.empty());
            } else if(from == folder) {
                new_name = to;
            }

            new_folders.insert(new_name);
        }
    }

    _parent->_ids = nullptr;
    std::swap(new_folders, _parent->_folders);
    std::swap(new_assets, _parent->_assets);

    return _parent->save_or_restore_tree();
}








FolderAssetStore::FolderAssetStore(const core::String& root) : _root(FileSystemModel::local_filesystem()->absolute(root).unwrap_or(root)), _filesystem(this) {
    y_profile();

    FileSystemModel::local_filesystem()->create_directory(_root).unwrap();

    reload_all().unwrap();
}

FolderAssetStore::~FolderAssetStore() {
}

core::String FolderAssetStore::asset_data_file_name(AssetId id) const {
    return _filesystem.join(_root, fmt("{}.asset", stringify_id(id)));
}

core::String FolderAssetStore::asset_desc_file_name(AssetId id) const {
    return _filesystem.join(_root, fmt("{}.desc", stringify_id(id)));
}


core::String FolderAssetStore::tree_file_name() const {
    const auto* fs = FileSystemModel::local_filesystem();
    return fs->join(_root, ".tree");
}

core::String FolderAssetStore::next_id_file_name() const {
    const auto* fs = FileSystemModel::local_filesystem();
    return fs->join(_root, ".next_id");
}

AssetStore::Result<FolderAssetStore::AssetDesc> FolderAssetStore::load_desc(AssetId id) const {
    y_profile();

    core::Vector<u8> buffer;
    if(auto file = io2::File::open(asset_desc_file_name(id));
        file.is_error() || file.unwrap().read_all(buffer).is_error()) {

        return core::Err(ErrorType::UnknownID);
    }

    AssetDesc desc;

    const char* data = reinterpret_cast<const char*>(buffer.data());
    for(usize i = 0; i != buffer.size(); ++i) {
        const char c = data[i];
        if(c != '\n') {
            desc.name.push_back(c);
        } else {
            const core::String leftover(data + i + 1, data + buffer.size());
            const std::string_view trimmed = core::trim(leftover);

            u32 type = 0;
            if(std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), type).ec == std::errc()) {
                desc.type = AssetType(type);
                return core::Ok(std::move(desc));
            }

            break;
        }
    }

    return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> FolderAssetStore::save_desc(AssetId id, const AssetDesc& desc) const {
    y_profile();

    const std::string_view data = fmt("{}\n{}\n", desc.name, desc.type);

    const core::String file_name = asset_desc_file_name(id);
    const core::String tmp_file = file_name + "_";

    if(auto file = io2::File::create(tmp_file);
       file.is_error() || file.unwrap().write_array(data.data(), data.size()).is_error()) {
        return core::Err(ErrorType::FilesytemError);
    }
    if(!FileSystemModel::local_filesystem()->rename(tmp_file, file_name)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

void FolderAssetStore::rebuild_id_map() const {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    if(!_ids) {
        _ids = std::make_unique<std::remove_reference_t<decltype(*_ids)>>();
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

    const auto lock = std::unique_lock(_lock);

    if(!_filesystem.create_directory(strict_parent_path(dst_name))) {
        return core::Err(ErrorType::FilesytemError);
    }

    if(_assets.find(dst_name) != _assets.end()) {
        return core::Err(ErrorType::NameAlreadyExists);
    }

    const AssetId id = next_id();
    const core::String data_file_name = asset_data_file_name(id);

    {
        y_profile_zone("writing");
        if(!io2::File::copy(data, data_file_name)) {
            return core::Err(ErrorType::FilesytemError);
        }
    }

    const AssetDesc desc = { dst_name, type };
    y_try(save_desc(id, desc));

    const auto it = _assets.emplace(dst_name, AssetData{id, type}).first;
    if(_ids) {
        (*_ids)[id] = it;
    }

    return core::Ok(id);
}

AssetStore::Result<> FolderAssetStore::write(AssetId id, io2::Reader& data) {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    const auto lock = std::unique_lock(_lock);

    const core::String data_file_name = asset_data_file_name(id);
    if(!io2::File::open(data_file_name)) {
        return core::Err(ErrorType::UnknownID);
    }

    if(!io2::File::copy(data, data_file_name)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<io2::ReaderPtr> FolderAssetStore::data(AssetId id) const {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    if(auto file = io2::File::open(asset_data_file_name(id))) {
        io2::ReaderPtr ptr = std::make_unique<io2::File>(std::move(file.unwrap()));
        return core::Ok(std::move(ptr));
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<AssetId> FolderAssetStore::id(std::string_view name) const {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    if(const auto it = _assets.find(name); it != _assets.end()) {
        return core::Ok(it->second.id);
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<core::String> FolderAssetStore::name(AssetId id) const {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    const auto lock = std::unique_lock(_lock);

    rebuild_id_map();
    if(const auto it = _ids->find(id); it != _ids->end()) {
        return core::Ok(core::String(it->second->first));
    }

    return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<> FolderAssetStore::remove(AssetId id) {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    const auto lock = std::unique_lock(_lock);

    auto na = name(id);
    y_try(na);

    if(!_filesystem.remove(na.unwrap())) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<> FolderAssetStore::rename(AssetId id, std::string_view new_name) {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    const auto lock = std::unique_lock(_lock);

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

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    const auto lock = std::unique_lock(_lock);

    rebuild_id_map();
    if(const auto it = _ids->find(id); it != _ids->end()) {
        return core::Ok(it->second->second.type);
    }

    return core::Err(ErrorType::UnknownID);
}


AssetId FolderAssetStore::next_id() {
    const auto lock = std::unique_lock(_lock);

    return AssetId::from_id(_next_id++);
}







FolderAssetStore::Result<> FolderAssetStore::load_tree() {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    _folders.clear();

    core::Vector<u8> tree_data;
    if(auto file = io2::File::open(tree_file_name()); file.is_error() || file.unwrap().read_all(tree_data).is_error()) {
        log_msg("Unable to open folder index", Log::Error);
        return core::Ok(); // ????
    }

    // Folders
    {
        core::String line;
        auto push_folder = [&] {
            if(!line.is_empty()) {
                y_debug_assert(is_valid_path(line));
                _folders.insert(std::move(line));
                line.make_empty();
            }
        };
        for(u8 b : tree_data) {
            const char c = char(b);
            if(c == '\n') {
                push_folder();
            } else {
                line.push_back(c);
            }
        }
        push_folder();
    }

    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::save_tree() const {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    core::String tree_data;
    {
        for(const core::String& folder : _folders) {
            y_debug_assert(is_valid_path(folder));
            y_debug_assert(std::string_view(folder) == strict_path(folder));
            y_debug_assert(_filesystem.is_directory(strict_parent_path(folder)).unwrap_or(false));

            tree_data += folder;
            tree_data += "\n";
        }
    }

    {
        const core::String file_name = tree_file_name();
        const core::String tmp_file = file_name + "_";

        Y_TODO(Openning file is slow, maybe we should cache it)
        if(auto file = io2::File::create(tmp_file); file.is_error() || file.unwrap().write_array(tree_data.data(), tree_data.size()).is_error()) {
            return core::Err(ErrorType::FilesytemError);
        }

        if(!FileSystemModel::local_filesystem()->rename(tmp_file, file_name)) {
            return core::Err(ErrorType::FilesytemError);
        }
    }

    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::save_or_restore_tree() {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    if(!save_tree()) {
        load_tree().unwrap();
        log_msg("Failed to save tree", Log::Error);
        return core::Err(ErrorType::FilesytemError);
    }
    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::load_asset_descs() {
    y_profile();

    core::DebugTimer _("Loading asset descs");

    const auto lock = std::unique_lock(_lock);

    _ids = nullptr;
    _assets.clear();

    core::Vector<u64> desc_ids;
    core::FlatHashMap<u64, usize> asset_sizes;

    const FileSystemModel* fs = FileSystemModel::local_filesystem();

    {
        y_profile_zone("Enumerating descs");
        const auto result = fs->for_each(_root, [&](const FileSystemModel::EntryInfo& info) {
            if(info.type != FileSystemModel::EntryType::File) {
                return;
            }

            auto uid_from_name = [](std::string_view name, std::string_view ext, u64& uid) {
                const usize size_without_ext = name.size() - ext.size();
                if(std::from_chars(name.data(), name.data() + size_without_ext, uid, 16).ec != std::errc()) {
                    return false;
                }
                return true;
            };

            if(info.name.ends_with(".desc")) {
                u64 uid = 0;
                if(uid_from_name(info.name, ".desc", uid)) {
                    desc_ids << uid;
                }
            }

            if(info.name.ends_with(".asset")) {
                u64 uid = 0;
                if(uid_from_name(info.name, ".asset", uid)) {
                    asset_sizes[uid] = info.file_size;
                }
            }
        });
    }


    core::Vector<core::Vector<std::pair<AssetDesc, AssetData>>> assets;
    {
        y_profile_zone("Reading descs");
        y_profile_msg(fmt_c_str("Reading descs for {} assets", desc_ids.size()));
        concurrent::StaticThreadPool thread_pool;

        const usize tasks = thread_pool.concurency() * 8 + 1;
        assets.set_min_size(tasks);

        y_profile_zone("schedule");
        usize index = 0;
        const usize split = (desc_ids.size() / (thread_pool.concurency() * 8)) + 1;
        for(usize i = 0; i < desc_ids.size(); i += split) {
            const core::Range range(desc_ids.begin() + i, desc_ids.begin() + std::min(desc_ids.size(), i + split));

            thread_pool.schedule([this, range, index, &assets, &asset_sizes] {
                y_profile_zone("Reading descs internal");
                for(const u64 uid : range) {
                    const AssetId id = AssetId::from_id(uid);
                    if(auto r = load_desc(id)) {
                        AssetDesc desc = r.unwrap();
                        AssetData data = { id, desc.type, 0 };

                        if(const auto it = asset_sizes.find(uid); it != asset_sizes.end()) {
                            data.file_size = it->second;
                        } else {
                            log_msg(fmt("\"{}\" has no asset file", desc.name), Log::Error);
                            continue;
                        }

                        assets[index].emplace_back(std::move(desc), std::move(data));
                    } else {
                        log_msg(fmt("{}.desc could not be read", stringify_id(id)), Log::Error);
                    }
                }
            });

            ++index;
        }
    }

    {
        y_profile_zone("Merging descs");
        usize emergency_id = 1;
        for(auto& a : assets) {
            for(auto& [desc, data] : a) {
                if(!_assets.emplace(desc.name, data).second) {
                    log_msg(fmt("\"{}\" already exists in asset database", desc.name), Log::Error);

                    {
                        fmt_into(desc.name, "_({})", emergency_id++);
                        _assets.emplace(desc.name, data);
                        save_desc(data.id, desc).ignore();
                    }
                }

                if(auto parent = _filesystem.parent_path(desc.name); parent && !parent.unwrap().is_empty()) {
                    if(_folders.insert(parent.unwrap()).second) {
                        log_msg(fmt("\"{}\" was not found in folder database", parent.unwrap()), Log::Warning);
                    }
                }
            }
        }
    }

    return core::Ok();
}

FolderAssetStore::Result<> FolderAssetStore::reload_all() {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    _next_id = u64(std::time(nullptr));
    load_tree().unwrap();
    load_asset_descs().unwrap();

    rebuild_id_map();

    return core::Ok();
}

}

