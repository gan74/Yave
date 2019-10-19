/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <y/test/test.h>
#include <y/mem/allocators.h>
#include <y/core/Vector.h>
#include <y/reflect/reflect.h>

#include <y/core/Chrono.h>

using namespace y;
using namespace memory;

y_test_func("Test test") {
	y_test_assert(true);
}

struct Refl {
	int x = 1;
	int y = 2;

	y_reflect(x, y)
};

void reflection() {
	{
		auto data = reflect::reflection_data<Refl*>();
		log_msg(fmt("% -> % (%)", data.type.name, data.members.size(), data.type.flags.is_pointer));
	}

	{
		auto data = reflect::reflection_data<Refl>();
		log_msg(fmt("% -> % (%)", data.type.name, data.members.size(), data.type.flags.is_pointer));
	}
}


#include <memory>

template<typename T, template<typename...> typename A>
using Vec = core::Vector<T, core::DefaultVectorResizePolicy, A<T>>;


int main() {

	reflection();

	/*int size = 1000000;

	{
		core::DebugTimer _("std::allocator");
		core::Vector<int> v;
		for(int i = 0; i != size; ++i) {
			v.emplace_back(i);
		}
	}
	{
		core::DebugTimer _("Allocator");
		Vec<int, StdAllocatorAdapter> v;
		for(int i = 0; i != size; ++i) {
			v.emplace_back(i);
		}
	}*/


	/*usize i = 1024;
	while(true) {
		log_msg(fmt("alloc: %KB", i / 1024));
		void* ptr = malloc(i);
		if(!ptr) {
			log_msg("FAILED!", Log::Error);
			break;
		}
		memset(ptr, 0, i);
		free(ptr);
		i *= 2;
		log_msg("OK!");
	}*/
	return 0;
}



