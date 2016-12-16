/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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

#include "utils.h"
#include <y/core/String.h>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#endif

namespace y {

namespace detail {
	usize StaticCounter::value = 0;

#ifdef __GNUG__
	core::String demangle_type_name(const char* name) {
	int status = 0;
	char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if(status) {
		return core::str(name);
	}

	return core::str_from_owned(d);
	}
#else
	core::String demangle_type_name(const char* name) {
		return core::str(name);
	}
#endif
}


Nothing fatal(const char* msg, const char* file, int line) {
	core::String msg_str = msg;
	if(file) {
		msg_str += " in file \""_s + file + "\"";
	}
	if(line) {
		msg_str += " at line "_s + line;
	}
	log_msg(msg_str, LogType::Error);
	exit(1);
}

}
