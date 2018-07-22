/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <y/io/File.h>
#include <y/io/BuffWriter.h>
#include <y/io/BuffReader.h>

#include <atomic>

#ifndef YAVE_NO_STDFS

namespace yave {

FolderAssetStore::FolderFileSystemModel::FolderFileSystemModel(std::string_view root) : _root(root) {
}

const core::String& FolderAssetStore::FolderFileSystemModel::root_path() const {
	return _root;
}

core::String FolderAssetStore::FolderFileSystemModel::current_path() const noexcept {
	return _root;
}

bool FolderAssetStore::FolderFileSystemModel::exists(std::string_view path) const noexcept {
	return LocalFileSystemModel::exists(path) && is_parent(_root, path);
}


FolderAssetStore::FolderAssetStore(std::string_view path) :
		_filesystem(path),
		_index_file_path(_filesystem.join(_filesystem.root_path(), ".index")) {

	log_msg("Store index file: " + _index_file_path);
	_filesystem.create_directory(path);
	read_index();
}

FolderAssetStore::~FolderAssetStore() {
	write_index();
}

const FileSystemModel* FolderAssetStore::filesystem() const {
	return &_filesystem;
}

void FolderAssetStore::read_index() {
	std::unique_lock lock(_lock);

	try {
		if(auto r = io::File::open(_index_file_path); r.is_ok()) {
			auto file = io::BuffReader(std::move(r.unwrap()));
			_from_id.clear();
			_from_name.clear();
			file.read_one(_next_id);
			while(!file.at_end()) {
				auto entry = std::make_unique<Entry>(serde::deserialized<Entry>(file));
				_from_id[entry->id] = entry.get();
				_from_name[entry->name] = std::move(entry);
			}
		}
	} catch(std::exception& e) {
		log_msg("Exception while reading index file: "_s + e.what(), Log::Error);
		log_msg(""_s + _from_id.size() + " assets imported"_s , Log::Error);
	}

	y_debug_assert(_from_id.size() == _from_name.size());
}

void FolderAssetStore::write_index() const {
	std::unique_lock lock(_lock);

	try {
		y_debug_assert(_from_id.size() == _from_name.size());
		if(auto r = io::File::create(_index_file_path); r.is_ok()) {
			auto file = io::BuffWriter(std::move(r.unwrap()));
			file.write_one(_next_id);
			for(const auto& row : _from_id) {
				const Entry& entry = *row.second;
				entry.serialize(file);
			}
		}
	} catch(std::exception& e) {
		log_msg("Exception while writing index file: "_s + e.what(), Log::Error);
	}
}

AssetId FolderAssetStore::import(io::ReaderRef data, std::string_view dst_name) {
	// remove/optimize
	auto flush_index = scope_exit([this] { write_index(); });
	std::unique_lock lock(_lock);

	auto& entry = _from_name[dst_name];
	if(entry) {
		y_throw("Asset already in store.");
	}

	auto dst_file = _filesystem.is_parent(_filesystem.root_path(), dst_name)
		? core::String(dst_name)
		: _filesystem.join(_filesystem.root_path(), dst_name);
	{
		auto dst_dir = _filesystem.parent_path(dst_file);
		if(!_filesystem.exists(dst_dir) && !_filesystem.create_directory(dst_dir)) {
			y_throw("Unable to create import directory.");
		}
	}

	if(!io::File::copy(data, dst_file)) {
		_from_name.erase(_from_name.find(dst_name));
		y_throw("Unable to import file as \""_s + dst_name + "\"");
	}

	AssetId id = ++_next_id;
	entry = std::make_unique<Entry>(Entry{dst_name, id});
	_from_id[id] = entry.get();

	y_debug_assert(_from_id.size() == _from_name.size());
	return id;
}


AssetId FolderAssetStore::id(std::string_view name) const {
	std::unique_lock lock(_lock);

	y_debug_assert(_from_id.size() == _from_name.size());
	if(auto it = _from_name.find(name); it != _from_name.end()) {
		return it->second->id;
	}
	y_throw("No asset with this name.");
}

io::ReaderRef FolderAssetStore::data(AssetId id) const {
	std::unique_lock lock(_lock);

	y_debug_assert(_from_id.size() == _from_name.size());
	if(auto it = _from_id.find(id); it != _from_id.end()) {
		auto filename = _filesystem.join(_filesystem.root_path(), it->second->name);
		if(auto r = io::File::open(filename); r.is_ok()){
			return io::ReaderRef(std::move(r.unwrap()));
		}
	}
	y_throw("No asset with this id.");
}

}

#endif // YAVE_NO_STDFS
