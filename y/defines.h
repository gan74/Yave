/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef Y_DEFINES_H
#define Y_DEFINES_H

namespace y {

struct Nothing;
Nothing fatal(const char* msg, const char* file = nullptr, int line = 0);

}


/*
// For some reasons this destroys sol2 (freeze at runtime)
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __func__
#endif
*/

#ifndef Y_PERF_LOG_DISABLED
#define Y_PERF_LOG_ENABLED
#endif

/****************** OS DEFINES BELOW ******************/

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WINDOWS)
#define Y_OS_WIN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(__linux__) || defined(__gnu_linux__)
#define Y_OS_LINUX
#endif



#endif // Y_DEFINES_H
