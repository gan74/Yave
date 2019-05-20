/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_UTILS_RECMACROS_H
#define Y_UTILS_RECMACROS_H

#include <y/defines.h>

/****************** RECURSIVE MACROS ******************/

#define Y_REC_MACRO(...) Y_MACRO_256(__VA_ARGS__)
/*#define Y_MACRO_1024(...) Y_MACRO_512(Y_MACRO_512(__VA_ARGS__))
#define Y_MACRO_512(...) Y_MACRO_256(Y_MACRO_256(__VA_ARGS__))*/
#define Y_MACRO_256(...) Y_MACRO_128(Y_MACRO_128(__VA_ARGS__))
#define Y_MACRO_128(...) Y_MACRO_64(Y_MACRO_64(__VA_ARGS__))
#define Y_MACRO_64(...) Y_MACRO_32(Y_MACRO_32(__VA_ARGS__))
#define Y_MACRO_32(...) Y_MACRO_16(Y_MACRO_16(__VA_ARGS__))
#define Y_MACRO_16(...) Y_MACRO_8(Y_MACRO_8(__VA_ARGS__))
#define Y_MACRO_8(...) Y_MACRO_4(Y_MACRO_4(__VA_ARGS__))
#define Y_MACRO_4(...) Y_MACRO_2(Y_MACRO_2(__VA_ARGS__))
#define Y_MACRO_2(...) Y_MACRO_1(Y_MACRO_1(__VA_ARGS__))
#define Y_MACRO_1(...) __VA_ARGS__

// http://jhnet.co.uk/articles/cpp_magic
#define Y_FIRST(a, ...) a
#define Y_SECOND(a, b, ...) b

#define Y_EMPTY()

#define Y_DEFER2(m) m Y_EMPTY Y_EMPTY()()

#define Y_IS_PROBE(...) Y_SECOND(__VA_ARGS__, 0)
#define Y_PROBE() ~, 1

#define Y_CAT(a, b) a ## b

#define Y_NOT(x) Y_IS_PROBE(Y_CAT(Y_NOT_, x))
#define Y_NOT_0 Y_PROBE()

#define Y_BOOL(x) Y_NOT(Y_NOT(x))

#define Y_IF_ELSE(condition) Y_IF_ELSE_(Y_BOOL(condition))
#define Y_IF_ELSE_(condition) Y_CAT(Y_IF_, condition)

#define Y_IF_1(...) __VA_ARGS__ Y_IF_1_ELSE
#define Y_IF_0(...)             Y_IF_0_ELSE

#define Y_IF_1_ELSE(...)
#define Y_IF_0_ELSE(...) __VA_ARGS__

//#define Y_HAS_ARGS(...) Y_BOOL(__VA_OPT__(1) 0)
#define Y_HAS_ARGS(...) Y_BOOL(Y_FIRST(Y_END_OF_ARGUMENTS __VA_ARGS__)())
#define Y_END_OF_ARGUMENTS(...) __VA_ARGS__ 0

#define Y_MACRO_MAP(m, first, ...)									\
	m(first)														\
	Y_IF_ELSE(Y_HAS_ARGS(__VA_ARGS__))								\
		(Y_DEFER2(Y_MACRO_MAP_)()(m, __VA_ARGS__))					\
		(/* Do nothing, just terminate */)

#define Y_MACRO_MAP_() Y_MACRO_MAP

#endif // Y_UTILS_RECMACROS_H
