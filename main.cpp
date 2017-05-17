
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

	core::Vector<Test> v(100000u, Test());
	/*int index = 0;
	for(auto& i : v) {
		i = index++;
	}*/

	std::atomic<usize> par_sum(0);
	usize sum = 0;
	{
		DebugTimer timer("parallel for");
		concurrent::parallel_for(v.begin(), v.end(), [&](auto&& i) {
			//par_sum += i;
			i.work();
		});
	}
	{
		DebugTimer timer("std::for_each");
		std::for_each(v.begin(), v.end(), [&](auto&& i) {
			//sum += i;
			i.work();
		});
	}

	std::cout << par_sum << " / " << sum << std::endl;

	concurrent::close_thread_pool();

	return 0;
}




