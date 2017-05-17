
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

using namespace y;
using namespace y::core;




int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	concurrent::init_thread_pool();

	core::Vector<int> v(10000000u, 0);
	int index = 0;
	for(auto& i : v) {
		i = index++;
	}

	std::atomic<usize> par_sum(0);
	usize sum = 0;
	{
		DebugTimer timer("parallel for");
		concurrent::parallel_for(v.begin(), v.end(), [&](auto&& i) {
			par_sum += i;
		});
	}
	{
		DebugTimer timer("std::for_each");
		std::for_each(v.begin(), v.end(), [&](auto&& i) {
			sum += i;
		});
	}
	std::cout << par_sum << "/" << sum << std::endl;

	for(usize i = 0; i != 1000000; ++i) {
		auto collect = concurrent::parallel_block_collect(0, 100, [](auto&& range) {
			int sum = 0;
			for(int i = range.begin(); i != range.end(); ++i) {
				sum += i;
			}
			return sum;
		});

		int total = 0;
		for(int i : collect) {
			total += i;
		}
		if(total != 4950) {
			fatal("Invalid result");
		}
		if(i % 250 == 0) {
			std::cout << i / 10000.0f << "%" << std::endl;
		}
		//std::cout << i << std::endl;
		//std::cout << total << " (" << collect.size() << " chunks)" << std::endl;
	}
	std::cout << "done." << std::endl;

	concurrent::close_thread_pool();

	return 0;
}




