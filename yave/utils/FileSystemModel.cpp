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

#include "FileSystemModel.h"

#include "filesystem.h"

namespace yave {


core::String FileSystemModel::extention(std::string_view path) const noexcept {
	for(usize i = path.size(); i != 0; --i) {
		if(path[i - 1] == '.') {
			return core::String(&path[i - 1], path.size() - i + 1);
		}
	}
	return "";
}

bool FileSystemModel::is_parent(std::string_view parent, std::string_view path) const noexcept {
	auto par = absolute(parent);
	auto f = absolute(path);
	return /*par.size() > f.size() &&*/ f.starts_with(par);
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

core::String LocalFileSystemModel::current_path() const noexcept {
	return fs::current_path().string();
}

core::String LocalFileSystemModel::parent_path(std::string_view path) const noexcept {
	return fs::path(path).parent_path().string();
}

core::String LocalFileSystemModel::filename(std::string_view path) const noexcept {
	return fs::path(path).filename().string();
}

bool LocalFileSystemModel::exists(std::string_view path) const noexcept {
	return fs::exists(path);
}

bool LocalFileSystemModel::is_directory(std::string_view path) const noexcept {
	return fs::is_directory(path);
}

core::String LocalFileSystemModel::join(std::string_view path, std::string_view name) const noexcept {
	if(!path.size()) {
		return name;
	}
	char last = path.back();
	core::String result;
	result.set_min_capacity(path.size() + name.size() + 1);
	result += path;
	if(last != '/' && last != '\\') {
		result.push_back('/');
	}
	result += name;
	return result;
}

core::String LocalFileSystemModel::absolute(std::string_view path) const noexcept {
	return fs::absolute(path).string();
}

void LocalFileSystemModel::for_each(std::string_view path, const for_each_f& func) const noexcept {
	try {
		for(auto& p : fs::directory_iterator(path)) {
			fs::path path = p.path().filename();
			auto str = path.string();
			func(str);
		}
	} catch(...) {
	}
}

bool LocalFileSystemModel::create_directory(std::string_view path) const noexcept {
	try {
		fs::create_directory(fs::path(path));
	} catch(...) {
		return false;
	}
	return true;
}

#endif

}
