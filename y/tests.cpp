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

#include <y/test/test.h>

#include <y/utils/log.h>

using namespace y;

#if 0
#include <y/core/Chrono.h>
#include <y/core/DenseMap.h>
#include <y/utils/format.h>
#include <unordered_map>

template<typename Map>
int bench(int max_key) {
	int sum = 0;

	double runs_d = 1000000.0 / max_key;
	const usize runs = runs_d < 1.0 ? 1 : usize(runs_d);

	for(usize r = 0; r != runs; ++r) {
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

		for(auto&& [k, v] : map) {
			sum += k;
			sum += v / 2;
		}
	}
	return sum;
}
#endif

int main() {
	const bool ok = test::run_tests();

	if(ok) {
		log_msg("All tests OK\n");
	} else {
		log_msg("Tests failed\n", Log::Error);
	}

#if 0
	for(int max_key = 1; max_key < 1024 * 1024; max_key *= 2) {
		log_msg(fmt("Size: %", max_key), Log::Debug);
		{
			core::DebugTimer _("ExternalDenseMap");
			bench<core::ExternalDenseMap<int, int>>(max_key);
		}

		{
			core::DebugTimer _("  StableDenseMap");
			bench<core::StableDenseMap<int, int>>(max_key);
		}

		{
			core::DebugTimer _("        DenseMap");
			bench<core::DenseMap<int, int>>(max_key);
		}

		{
			core::DebugTimer _("   unordered_map");
			bench<std::unordered_map<int, int>>(max_key);
		}
	}
#endif

	return 0;
}



