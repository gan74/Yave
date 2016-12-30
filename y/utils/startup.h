/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
