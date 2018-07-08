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

#ifndef YAVE_NO_STDFS

core::String FileSystemModel::current_path() const {
	return fs::current_path().string();
}

core::String FileSystemModel::parent_path(std::string_view path) const {
	return fs::path(path).parent_path().string();
}

core::String FileSystemModel::filename(std::string_view path) const {
	return fs::path(path).filename().string();
}

bool FileSystemModel::exists(std::string_view path) const {
	return fs::exists(path);
}

bool FileSystemModel::is_directory(std::string_view path) const {
	return fs::is_directory(path);
}

core::String FileSystemModel::join(std::string_view path, std::string_view name) const {
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

void FileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
	for(auto& p : fs::directory_iterator(path)) {
		fs::path path = p.path().filename();
		auto str = path.string();
		func(str);
	}
}

#endif


}
