
#include <y/utils.h>
#include <y/core/Vector.h>
#include <y/core/String.h>

#include <y/concurrent/StaticThreadPool.h>
#include <y/concurrent/concurrent.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>
#include <y/utils/perf.h>

#include <iostream>

#include <mutex>
#include <condition_variable>
#include <thread>

#include <cmath>
#include <vector>
#include <list>
#include <random>
#include <future>

using namespace y;
using namespace y::core;
using namespace y::concurrent;


void slow(usize m = 5000) {
	for(volatile usize j = 0; j != 100; ++j) {
		for(volatile usize i = 0; i != m; ++i) {
		}
	}
}

void very_slow(usize m) {
	for(volatile usize j = 0; j != 100; ++j) {
		slow(m);
	}
}


int main() {
	usize iterations = 10000;
	core::Vector<int> v(iterations, 0);
	int index = 0;
	for(auto& i : v) {
		i = index++;
	}

	concurrent::init_thread_pool();
	{
		DebugTimer timer("parallel_for_each(" + str(concurrent::concurency()) + ")");
		concurrent::parallel_for_each(v.begin(), v.end(), [&](auto&&) {
			slow();
		});
	}

	StaticThreadPool pool;
	{
		DebugTimer timer("pool for_each(" + str(concurrent::concurency()) + ")");
		pool.parallel_for_each(v.begin(), v.end(), [&](auto&&) {
			slow();
		});
		pool.process_until_empty();
	}


	return 0;
}




