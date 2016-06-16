/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "Dir.h"
#include "File.h"
#include <dirent.h>
#include <climits>
#include <n/defines.h>

#ifdef N_OS_WIN
#include <windows.h>
char *realpath(const char *restrict file_name, char *restrict resolved_name) {
	return _fullpath(resolved_name, file_name, _MAX_PATH);
}
#endif

namespace n {
namespace io {

Dir::Dir(const core::String &dirName) : name(dirName), full(dirName) {
	DIR *dir = opendir(name.data());
	if(dir) {
		char buffer[PATH_MAX + 1];
		full = realpath(name.data(), buffer);
		dirent *s = readdir(dir);
		while(s) {
			content.append(s->d_name);
			s = readdir(dir);
		}
		closedir(dir);
	}

}

const core::String &Dir::getName() const {
	return name;
}

const core::String &Dir::getPath() const {
	return full;
}

core::Array<core::String> Dir::getContent() const {
	return content;
}

core::Array<core::String> Dir::getSubDirs() const {
	return content.filtered([=](const core::String &n) {
		return n != "." && n != ".." && !File::exists(full + "/" + n);
	});
}

core::Array<core::String> Dir::getFiles() const {
	return content.filtered([=](const core::String &n) {
		return File::exists(full + "/" + n);
	});
}



bool Dir::exists(const core::String &dir) {
	DIR *d = opendir(dir.data());
	if(d) {
		closedir(d);
	}
	return d;
}

}
}
