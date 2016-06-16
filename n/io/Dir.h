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

#ifndef N_IO_DIR_H
#define N_IO_DIR_H

#include <n/utils.h>
#include <n/core/String.h>
#include <n/core/Array.h>

namespace n {
namespace io {

class Dir
{
	public:
		Dir(const core::String &dirName);

		const core::String &getName() const;
		const core::String &getPath() const;
		core::Array<core::String> getContent() const;
		core::Array<core::String> getSubDirs() const;
		core::Array<core::String> getFiles() const;

		static bool exists(const core::String &dir);

	private:
		core::String name;
		core::String full;
		core::Array<core::String> content;
};

}// io
}// n


#endif // N_IO_DIR_H

