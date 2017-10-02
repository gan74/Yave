
#include <y/utils.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/concurrent/concurrent.h>

#include <y/core/Chrono.h>

#include <iostream>

#include <mutex>
#include <condition_variable>
#include <thread>

#include <cmath>
#include <vector>
#include <list>
#include <random>

using namespace y;
using namespace y::core;

void slow(usize m) {
	for(volatile usize j = 0; j != 100; ++j) {
		for(volatile usize i = 0; i != m; ++i) {
		}
	}
}


int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	usize iterations = 10000;
	core::Vector<int> v(iterations, 0);
	int index = 0;
	for(auto& i : v) {
		i = index++;
	}

	{
		DebugTimer timer("parallel_for_each(1)");
		concurrent::parallel_for_each(v.begin(), v.end(), [&](auto&& i) {
			slow(i);
		});
	}

	{
		DebugTimer timer("std::for_each");
		std::for_each(v.begin(), v.end(), [&](auto&& i) {
			slow(i);
		});
	}

	concurrent::init_thread_pool();

	{
		DebugTimer timer("parallel_for_each(" + str(concurrent::concurency()) + ")");
		concurrent::parallel_for_each(v.begin(), v.end(), [&](auto&& i) {
			slow(i);
		});
	}

	{
		std::atomic<usize> sum(0);
		DebugTimer timer("parallel_for_each sum(n)");
		concurrent::parallel_for_each(v.begin(), v.end(), [&](auto&& i) {
			sum += i;
		});
		std::cout << sum << std::endl;
	}



	return 0;
}




