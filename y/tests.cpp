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


#if 1
#include <y/core/Chrono.h>
#include <y/core/Vector.h>
#include <y/core/DenseMap.h>
#include <y/utils/format.h>
#include <y/utils/name.h>
#include <y/math/random.h>

#include <unordered_map>
#include <random>
#include <cmath>

template<usize B>
struct BadHash {
	template<typename T>
	usize operator()(const T& k) const {
		std::hash<T> hasher;
		usize h = hasher(k);
		return h % B;
	}
};



template<template<typename...> typename Map>
static auto bench_fill(usize count = 10000000) {
	Map<usize, usize> map;
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	return map;
}


template<template<typename...> typename Map>
static auto bench_reserve_fill(usize count = 10000000) {
	Map<usize, usize> map;
	map.reserve(count);
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	return map;
}


template<template<typename...> typename Map>
static auto bench_fill_iter_50(usize count = 5000000) {
	Map<usize, usize> map;
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}

	usize a = 0;
	for(usize i = 0; i != 25; ++i) {
		for(const auto& [k, v] : map) {
			a += k * i;
		}
	}
	map.insert({a, a});

	a = 0;
	for(usize i = 0; i != 25; ++i) {
		for(const auto& [k, v] : map) {
			a += v * i;
		}
	}
	map.insert({a, a});

	return map;
}


template<template<typename...> typename Map>
static auto bench_fill_iter_50_tiny() {
	return bench_fill_iter_50<Map>(100);
}

template<template<typename...> typename Map>
static auto bench_fill_erase_all(usize count = 1000000) {
	Map<usize, usize> map;
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	for(usize i = 0; i != count; ++i) {
		map.erase(map.find(i));
	}
	return map;
}

template<template<typename...> typename Map>
static auto bench_fill_erase_refill(usize count = 1000000) {
	Map<usize, usize> map;
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	for(usize i = 0; i != count; ++i) {
		map.erase(map.find(i));
	}
	for(usize i = 0; i != count; ++i) {
		map.insert({i * 2, i});
	}
	return map;
}

template<template<typename...> typename Map, typename Hasher = std::hash<usize>>
static auto bench_fill_find_all_50_50(usize count = 1000000) {
	Map<usize, usize, Hasher> map;

	math::FastRandom rng;
	std::uniform_int_distribution<usize> dist(0, count * 2);

	for(usize i = 0; i != count; ++i) {
		map.insert({dist(rng), i});
	}

	usize sum = 0;
	for(usize i = 0; i != count * 2; ++i) {
		if(const auto& it = map.find(i); it != map.end()) {
			sum += i;
		}
	}
	map.insert({sum, sum});
	return map;
}



template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_huge(usize count = 10000000) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_medium(usize count = 10000) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_tiny(usize count = 100) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_badhash_8(usize count = 10000) {
	return bench_fill_find_all_50_50<Map, BadHash<8>>(count);
}


using result_type = core::Vector<std::tuple<const char*, double, usize>>;

template<template<typename...> typename Map>
result_type bench_implementation() {
	result_type results;
	core::DebugTimer _(ct_type_name<Map<int, int>>());

#define BENCH_ONE(func)																	\
	do {																				\
		try {																			\
			core::Chrono chrono;														\
			for(usize count = 1; true; ++count) {										\
				const usize res = func<Map>().size();									\
				const auto elapsed = chrono.elapsed();									\
				if(elapsed.to_secs() >= 1.0) {											\
					results.emplace_back(#func, elapsed.to_secs() / count, res);		\
					break;																\
				}																		\
			}																			\
		} catch(std::exception& e) {													\
			y_fatal("Exception while running % for %:\n%",								\
				#func, ct_type_name<Map<int, int>>(), e.what());						\
		}																				\
	} while(false)

	BENCH_ONE(bench_fill);
	BENCH_ONE(bench_reserve_fill);
	BENCH_ONE(bench_fill_iter_50);
	BENCH_ONE(bench_fill_iter_50_tiny);
	BENCH_ONE(bench_fill_erase_all);
	BENCH_ONE(bench_fill_erase_refill);
	BENCH_ONE(bench_fill_find_all_50_50_huge);
	BENCH_ONE(bench_fill_find_all_50_50);
	BENCH_ONE(bench_fill_find_all_50_50_medium);
	BENCH_ONE(bench_fill_find_all_50_50_tiny);
	BENCH_ONE(bench_fill_find_all_50_50_badhash_8);

#undef BENCH_ONE

	return results;
}



#endif


int main() {
	const bool ok = test::run_tests();

	if(ok) {
		log_msg("All tests OK\n");
	} else {
		log_msg("Tests failed\n", Log::Error);
	}

#if 1
	core::Vector<std::pair<const char*, result_type>> results;
	log_msg("Benching...");
	results.emplace_back("ExternalDenseMap", bench_implementation<core::ExternalDenseMap>());
	results.emplace_back("DenseMap", bench_implementation<core::DenseMap>());
	results.emplace_back("std::unordered_map", bench_implementation<std::unordered_map>());
	log_msg("Done\n");

	for(const auto& impl : results) {
		const usize result_count = impl.second.size();
		log_msg(fmt("%:", impl.first), Log::Perf);
		for(usize r = 0; r != result_count; ++r) {
			const auto res = impl.second[r];
			core::String line = "    ";
			line += std::get<0>(res);
			while(line.size() < 48) {
				line += " ";
			}
			fmt_into(line, "% s ", std::get<1>(res));



			core::Vector<double> times;
			for(const auto& i : results) {
				times << std::get<1>(i.second[r]);
			}
			std::sort(times.begin(), times.end());

			const double res_time = std::get<1>(res);
			if(res_time <= times[1] && times[1] * 0.95 < times[0]) {
				line += "~";
			}
			if(res_time > times[0] * 2.0) {
				line += "!";
			}
			if(times[0] == res_time) {
				line += "*";
				if(times[1] * 0.5 > res_time) {
					line += "*";
				}
			}
			log_msg(line, Log::Perf);
		}
	}


#endif

	return 0;
}



