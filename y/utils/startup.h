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
#ifndef Y_UTILS_STARTUP_H
#define Y_UTILS_STARTUP_H

/*
#define Y_ON_START_LINE_HELPER(prefix, LINE) _on_startup_ ## prefix ## _at_ ## LINE
#define Y_ON_START_HELPER(prefix, LINE) Y_ON_START_LINE_HELPER(prefix, LINE)
#define Y_START_FUNC Y_ON_START_HELPER(func, __LINE__)
#define Y_START_RUNNER Y_ON_START_HELPER(runner, __LINE__)


#define y_on_startup()																					\
static void Y_START_FUNC();																				\
namespace {																								\
	class Y_START_RUNNER {																				\
		Y_START_RUNNER() {																				\
			Y_START_FUNC();																				\
		}																								\
		static Y_START_RUNNER runner;																	\
	};																									\
	Y_START_RUNNER Y_START_RUNNER::runner = Y_START_RUNNER();											\
}																										\
void Y_START_FUNC()
*/

#endif // Y_UTILS_STARTUP_H
