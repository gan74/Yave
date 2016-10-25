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
#ifndef Y_DEFINES_H
#define Y_DEFINES_H

namespace y {

struct Nothing;
Nothing fatal(const char* msg, const char* file = nullptr, int line = 0);

}


#define Y_TODO(msg) /* msg */


#ifndef __PRETTY_FUNCTION__
#define __FUNC__ __PRETTY_FUNCTION__
#endif

/****************** OS DEFINES BELOW ******************/

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WINDOWS)
#define Y_OS_WIN
#define Y_OS_WIN
#define WIN32_LEAN_AND_MEAN
#endif


#endif // Y_DEFINES_H
