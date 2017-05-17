
#include <y/utils.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/concurrent/concurrent.h>

#include <y/core/Chrono.h>

#include <iostream>

#include <mutex>
#include <condition_variable>
#include <thread>


using namespace y;
using namespace y::core;

struct Test {
	void work() {
		for(volatile int i = 0; i != 1000; ++i) {
		}
	}
};


int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	concurrent::init_thread_pool();

	core::Vector<int> v(100000u, 0);
	int index = 0;
	for(auto& i : v) {
		i = index++;
	}
	log_msg("main = " + core::str(std::this_thread::get_id()));

	auto c = concurrent::parallel_block_collect(v.begin(), v.end(), [](auto&& r) {
		log_msg("[" + str(*r.begin()) + ", " + str(*(r.end() - 1)) + "]");
		usize s = 0;
		for(auto i : r) {
			s += i;
		}
		return s;
	});

	usize sum = 0;
	for(auto i : c) {
		std::cout << i << std::endl;
		sum += i;
	}
	std::cout << sum << std::endl;


	concurrent::close_thread_pool();

	return 0;
}




