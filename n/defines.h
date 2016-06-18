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

#ifndef N_DEFINES_H
#define N_DEFINES_H

namespace n {

class Nothing;

void fatal(const char *msg, const char *file = 0, int line = 0);

template<typename... A>
void unused(A...) {}


namespace core {
	class String2;
	using String = String2;
}

}

/* defines stuffs here */
#ifndef __GNUC__
#define N_NO_FORCE_INLINE
#endif

#ifdef N_DEBUG
#define N_NO_FORCE_INLINE
#endif

#ifdef N_NO_FORCE_INLINE
#define N_FORCE_INLINE inline
#else
#define N_FORCE_INLINE inline  __attribute__((always_inline))
#endif

#ifdef __restrict__
	#define restrict __restrict__
#else
	#ifdef __restrict
		#define restrict __restrict
	#else
		#define restrict
	#endif
#endif

#ifdef __GNUC__
#if __GNUC__ <= 4
#define N_GCC_4
#endif
#endif

#ifndef __PRETTY_FUNCTION__
#define __FUNC__ __PRETTY_FUNCTION__
#endif

/****************** OS DEFINES BELOW ******************/

#ifdef __WIN32
#define N_OS_WIN
#endif

#ifdef __WIN32__
#define N_OS_WIN
#endif

#ifdef WIN32
#define N_OS_WIN
#endif

#ifdef _WINDOWS
#define N_OS_WIN
#endif



/****************** COMPILATION DEFINES BELOW ******************/


//#define N_AUTO_TEST

#endif //N_DEFINES_H
