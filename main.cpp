#include <y/concurrent/WorkGroup.h>

#include <y/core/Chrono.h>

#include <iostream>

using namespace y;
using namespace y::concurrent;

void test() {
	WorkGroup w;

	for(usize i = 0; i != 800; ++i) {
		w.schedule([]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		});
	}
}


int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	core::Chrono ch;
	test();
	std::cout << "DONE in " << ch.elapsed().to_secs() << "s" << std::endl;

	return 0;
}




