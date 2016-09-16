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

#include "log.h"
#include <y/utils.h>
#include <y/concurrent/Mutex.h>
#include <y/concurrent/LockGuard.h>

#include <iostream>

namespace y {

static constexpr std::array<const char*, 4> log_type_str = {"info", "warning", "error", "debug"};

void log_msg(const char* msg, LogType type) {
	static concurrent::Mutex lock;
	auto _ = concurrent::lock(lock);
	(type == LogType::Error ? std::cerr : std::cout) << "[" << log_type_str[usize(type)] << "] " << msg << std::endl;
}

}
