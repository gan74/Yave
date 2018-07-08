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

static AssetId next_id() {
	static std::atomic<AssetId> id = assets::invalid_id;
	return ++id;
}

static fs::path clean(fs::path path) {
	fs::path cl;
	for(auto& p : fs::absolute(path)) {
		if(p == ".") {
		} else if(p == "..") {
			cl = cl.remove_filename();
		} else {
			cl /= p;
		}
	}
	return cl;
}

static fs::copy_options copy_options(AssetStore::ImportType type) {
	switch(type) {
		case AssetStore::Intern:
			return fs::copy_options::none;
		case AssetStore::Reference:
			return fs::copy_options::create_hard_links;
		default:
			break;
	}
	return y_fatal("Unsupported import type.");
}



FolderAssetStore::FolderAssetStore(std::string_view path) : _path(path), _index_file_path(clean(file_path(".index")).string()) {
	log_msg("Store index file: " + _index_file_path);
	fs::create_directory(_path);
	read_index();
}

FolderAssetStore::~FolderAssetStore() {
	write_index();
}

fs::path FolderAssetStore::file_path(std::string_view name) const {
	return (fs::path(_path) /= name);
}

void FolderAssetStore::read_index() {
	std::unique_lock lock(_lock);

	try {
		if(auto r = io::File::open(_index_file_path); r.is_ok()) {
			auto file = io::BuffReader(std::move(r.unwrap()));
			_from_id.clear();
			_from_name.clear();
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

void FolderAssetStore::write_index() {
	std::unique_lock lock(_lock);

	try {
		y_debug_assert(_from_id.size() == _from_name.size());
		auto index = io::BuffWriter(std::move(io::File::create(_index_file_path).unwrap()));
		for(const auto& row : _from_id) {
			const Entry& entry = *row.second;
			entry.serialize(index);
		}
	} catch(std::exception& e) {
		log_msg("Exception while writing index file: "_s + e.what(), Log::Error);
	}
}

void FolderAssetStore::try_import(std::string_view src, std::string_view dst, ImportType import_type) {
	auto dst_file = file_path(dst);
	{
		auto dst_dir = dst_file.parent_path();
		if(!fs::exists(dst_dir) && !fs::create_directory(dst_dir)) {
			y_throw("Unable to create import directory.");
		}
	}

#ifdef YAVE_STDFS_BAD_COPY
	if(import_type == ImportType::Intern) {
		if(!io::File::copy(src, dst_file.string())) {
			y_throw("Unable to copy file.");
		}
	} else
#endif
	{
		fs::copy_file(src, dst_file, copy_options(import_type));
	}
}


AssetId FolderAssetStore::import_as(std::string_view src_name, std::string_view dst_name, ImportType import_type) {
	// remove/optimize
	auto flush_index = scope_exit([this] { write_index(); });
	std::unique_lock lock(_lock);

	auto& entry = _from_name[dst_name];
	if(entry) {
		y_throw("Asset already in store.");
	}

	try {
		try_import(src_name, dst_name, import_type);
	} catch(...) {
		_from_name.erase(_from_name.find(dst_name));
		throw;
	}

	AssetId id = next_id();
	entry = std::make_unique<Entry>(Entry{dst_name, id});
	_from_id[id] = entry.get();
	_from_name[dst_name] = std::move(entry);
	y_debug_assert(_from_id.size() == _from_name.size());
	return id;
}


AssetId FolderAssetStore::id(std::string_view name) {
	std::unique_lock lock(_lock);

	y_debug_assert(_from_id.size() == _from_name.size());
	if(auto it = _from_name.find(name); it != _from_name.end()) {
		return it->second->id;
	}
	y_throw("No asset with this name.");
}

io::ReaderRef FolderAssetStore::data(AssetId id) {
	std::unique_lock lock(_lock);

	y_debug_assert(_from_id.size() == _from_name.size());
	if(auto it = _from_id.find(id); it != _from_id.end()) {
		fs::path filename = file_path(it->second->name);
		if(auto r = io::File::open(filename.string()); r.is_ok()){
			return io::ReaderRef(std::move(r.unwrap()));
		}
	}
	y_throw("No asset with this id.");
}

}

#endif // YAVE_NO_STDFS
