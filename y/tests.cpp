/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <y/core/Vector.h>
#include <y/core/DenseMap.h>

#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


#include <y/test/test.h>

using namespace y;
using namespace y::core;

template<typename Map>
void bench() {
	static constexpr int max_key = 100000;

	Map map;
	for(int i = 0; i != max_key; ++i) {
		map.insert({i, i * 2});
	}

	usize map_size = max_key;
	unused(map_size);
	for(int i = 0; i < max_key; i += 1 + (i / 8)) {
		const auto it = map.find(i);
		y_debug_assert(it != map.end());
		map.erase(it);
		--map_size;
		y_debug_assert(map_size == map.size());
	}

	int sum = 0;
	for(auto&& [k, v] : map) {
		sum += k;
		sum += v / 2;
	}

	volatile int s = 0;
	s = sum;
	unused(s);
}


int main() {
	test::run_tests();

	using namespace y::core::split;

	static constexpr int max_key = 10;
	DenseMap<int, int> map;

	for(int i = 0; i != max_key; ++i) {
		map.emplace(i, i * 2);
	}

	y_debug_assert(map.contains(4));
	y_debug_assert(!map.contains(max_key + 1));
	y_debug_assert(map.find(max_key + 1) == map.end());

	log_msg("values:");
	for(auto&& i : map.values()) {
		log_msg(fmt("  %", i));
	}

	log_msg("keys:");
	for(auto&& i : map.keys()) {
		log_msg(fmt("  %", i));
		y_debug_assert(map.contains(i));
	}

	log_msg("key-values:");
	for(auto&& [k, v] : map.key_values()) {
		log_msg(fmt("  % -> %", k, v));
	}

	log_msg("for-each:");
	for(auto&& [k, v] : map) {
		log_msg(fmt("  % -> %", k, v));
	}


	{
		{
			core::DebugTimer _("     DenseMap");
			bench<DenseMap<int, int>>();
		}

		{
			core::DebugTimer _("unordered_map");
			bench<std::unordered_map<int, int>>();
		}
	}

	log_msg("Ok");
	return 0;
}



