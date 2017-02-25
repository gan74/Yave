#include <y/concurrent/WorkGroup.h>
#include <y/core/Chrono.h>
#include <y/concurrent/SpinLock.h>


#include <unordered_map>
#include <iostream>

using namespace y;
using namespace y::concurrent;

static std::unordered_map<std::thread::id, usize> result;
static SpinLock mutex;

void done() {
	std::lock_guard<decltype(mutex)> _(mutex);
	auto x = result.insert(std::make_pair(std::this_thread::get_id(), 1));
	if(!x.second) {
		(*x.first).second++;
	}
}

void test() {
	core::Vector<int> v;
	for(int i = 0; i != 100000; ++i) {
		v << i;
	}

	WorkGroup(10).schedule_range(v.begin(), v.end(),
		[](int) {
			done();
		});
}


int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	core::Chrono ch;
	test();
	std::cout << "DONE in " << ch.elapsed().to_secs() << "s" << std::endl;


	usize min = 10000;
	usize max = 0;
	for(auto e : result)  {
		usize s = e.second;
		min = std::min(min, s);
		max = std::max(max, s);
	}

	std::cout << "partition size in [" << min << ", " << max << "]" << std::endl;

	usize total = 0;
	usize id = 0;
	for(auto e : result)  {
		std::cout << "thread #" << id++ << " = " << e.second << std::endl;
		total += e.second;
	}

	std::cout << "total = " << total << std::endl;


	return 0;
}




