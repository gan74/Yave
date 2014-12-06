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

#include "types.h"
#include <n/core/String.h>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

n::core::String demangle(const char* name) {
	int status = 0;
	const char *d = abi::__cxa_demangle(name, NULL, NULL, &status);
	return !status ? d : name;
}

#else

core::String demangle(const char* name) {
	return name;
}

#endif

namespace n {
	uint typeId = 0;

	core::String Type::name() const {
		return demangle(info->name());
	}
}


