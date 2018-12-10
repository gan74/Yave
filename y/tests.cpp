#include <y/test/test.h>
#include <y/mem/allocators.h>
#include <y/core/Vector.h>

#include <y/core/Chrono.h>

using namespace y;
using namespace memory;

y_test_func("Test test") {
	y_test_assert(true);
}


#include <memory>

template<typename T, template<typename...> typename A>
using Vec = core::Vector<T, core::DefaultVectorResizePolicy, A<T>>;


int main() {

	int size = 1000000;

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
	}


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



