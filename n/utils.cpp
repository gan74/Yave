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


#include "utils.h"
#include <n/core/String.h>
#include <n/concurrent/Atomic.h>
#include <n/defines.h>
#include <ctime>
#include <iostream>

namespace n {

Nothing fatal(const char *msg, const char *file, int line) {
	std::cerr<<msg;
	if(file) {
		std::cerr<<" in file "<<file;
		if(line) {
			std::cerr<<" at line "<<line;
		}
	}
	std::cerr<<std::endl;
	exit(1);
	return Nothing();
}

uint uniqueId() {
	static concurrent::auint counter;
	return ++counter;
}

}
