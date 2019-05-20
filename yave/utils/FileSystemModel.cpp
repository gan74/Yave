/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "FileSystemModel.h"

#include "filesystem.h"

namespace yave {

FileSystemModel::Result<core::String> FileSystemModel::parent_path(std::string_view path) const {
	return absolute(join(path, ".."));
}

core::String FileSystemModel::extention(std::string_view path) const {
	for(usize i = path.size(); i != 0; --i) {
		if(path[i - 1] == '.') {
			return core::String(&path[i - 1], path.size() - i + 1);
		}
	}
	return "";
}

FileSystemModel::Result<bool> FileSystemModel::is_parent(std::string_view parent, std::string_view path) const {
	auto par = absolute(parent);
	if(!par) {
		return core::Err();
	}
	auto f = absolute(path);
	if(!f) {
		return core::Err();
	}
	return core::Ok(f.unwrap().starts_with(par.unwrap()));
}

const FileSystemModel* FileSystemModel::local_filesystem() {
#ifndef YAVE_NO_STDFS
	static LocalFileSystemModel filesystem;
	return &filesystem;
#else
	return y_fatal("FileSystemModel::local_filesystem() is not supported");
#endif
}


#ifndef YAVE_NO_STDFS

FileSystemModel::Result<core::String> LocalFileSystemModel::current_path() const {
	try {
		return core::Ok(canonicalize(fs::current_path().string()));
	} catch(...) {
	}
	return core::Err();
}

core::String LocalFileSystemModel::filename(std::string_view path) const {
	try {
		return fs::path(path).filename().string();
	} catch(...) {
	}
	return path;
}

FileSystemModel::Result<bool> LocalFileSystemModel::exists(std::string_view path) const {
	try {
		return core::Ok(fs::exists(path));
	} catch(...) {
	}
	return core::Err();
}

FileSystemModel::Result<bool> LocalFileSystemModel::is_directory(std::string_view path) const {
	try {
		return core::Ok(fs::is_directory(path));
	} catch(...) {
	}
	return core::Err();
}

core::String LocalFileSystemModel::join(std::string_view path, std::string_view name) const {
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

FileSystemModel::Result<core::String> LocalFileSystemModel::absolute(std::string_view path) const {
	try {
		return core::Ok(canonicalize(fs::absolute(path).string()));
	} catch(...) {
	}
	return core::Err();
}

FileSystemModel::Result<> LocalFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
	try {
		for(auto& dir : fs::directory_iterator(path)) {
			fs::path p = dir.path().filename();
			auto str = p.string();
			func(str);
		}
	} catch(...) {
	}
	return core::Err();
}

FileSystemModel::Result<> LocalFileSystemModel::create_directory(std::string_view path) const {
	try {
		fs::create_directory(fs::path(path));
		return core::Ok();
	} catch(...) {
	}
	return core::Err();
}

FileSystemModel::Result<> LocalFileSystemModel::remove(std::string_view path) const {
	try {
		fs::remove_all(fs::path(path));
		return core::Ok();
	} catch(...) {
	}
	return core::Err();
}

FileSystemModel::Result<> LocalFileSystemModel::rename(std::string_view from, std::string_view to) const {
	try {
		fs::rename(fs::path(from), fs::path(to));
		return core::Ok();
	} catch(...) {
	}
	return core::Err();
}

bool LocalFileSystemModel::is_delimiter(char c) const {
	return c == '\\' || c == '/';
}

core::String LocalFileSystemModel::canonicalize(core::String path) const {
	for(auto& c : path) {
		if(is_delimiter(c)) {
			c = '/';
		}
	}
	return path;
}

#endif

}
