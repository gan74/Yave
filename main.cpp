
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

void slow(usize m) {
	for(volatile usize i = 0; i != m; ++i) {
	}
}

int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	concurrent::init_thread_pool();

	core::Vector<int> v(5000u, 0);
	int index = 0;
	for(auto& i : v) {
		i = index++;
	}

	/*{
		Chrono timer;
		usize loops = 0;
		while(timer.elapsed().to_secs() < 10) {
			concurrent::parallel_for_each(v.begin(), v.end(), [&](auto&& i) {
				slow(i);
			});
			loops++;
		}
		std::cout << "parallel_for_each: " << loops << std::endl;
	}

	{
		Chrono timer;
		usize loops = 0;
		while(timer.elapsed().to_secs() < 10) {
			volatile usize sum = 0;
			concurrent::parallel_block_for(0, 100, [&](auto&&) {
				sum += 1;
			});
			loops++;
		}
		std::cout << "parallel_block_for spam: " << loops << std::endl;;
	}

	{
		Chrono timer;
		usize loops = 0;
		while(timer.elapsed().to_secs() < 10) {
			auto col = concurrent::parallel_collect(v.begin(), v.end(), [&](auto&& range) {
				core::Vector<int> v;
				for(auto i : range) {
					v.push_back(i);
				}
				return v;
			});
			std::sort(col.begin(), col.end());
			if(v != col) {
				fatal("invalid result");
			}
			loops++;
		}
		std::cout << "parallel_collect: " << loops << std::endl;;
	}*/

	auto thread = new std::thread([]() {
		concurrent::parallel_for(0, 1, [](auto&&) {
			std::cout << "started..." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(10));
			std::cout << "done!" << std::endl;
		});
	});
	thread->detach();

	return 0;
}




