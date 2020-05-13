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

//#define BENCH
#ifndef Y_DEBUG
#define BENCH
#endif

#ifdef BENCH
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

#ifndef Y_DEBUG
static constexpr usize bench_count_mul = 1000; // default = 1000
#else
static constexpr usize bench_count_mul = 1; // for faster debug
#endif


template<template<typename...> typename Map>
static auto bench_reserve(usize count = 10000 * bench_count_mul) {
	Map<usize, usize> map;
	map.reserve(count);
	map.insert({count / 2, 4});
	return map;
}

template<template<typename...> typename Map>
static auto bench_fill(usize count = 10000 * bench_count_mul) {
	Map<usize, usize> map;
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	return map;
}


template<template<typename...> typename Map>
static auto bench_reserve_fill(usize count = 10000 * bench_count_mul) {
	Map<usize, usize> map;
	map.reserve(count);
	for(usize i = 0; i != count; ++i) {
		map.insert({i, i * 2});
	}
	return map;
}


template<template<typename...> typename Map>
static auto bench_fill_iter_50(usize count = 5000 * bench_count_mul) {
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
static auto bench_fill_iter_50_medium() {
	return bench_fill_iter_50<Map>(100000);
}


template<template<typename...> typename Map>
static auto bench_fill_iter_50_tiny() {
	return bench_fill_iter_50<Map>(100);
}


template<template<typename...> typename Map>
static auto bench_fill_iter_empty_50(usize count = 1000 * bench_count_mul) {
	Map<usize, usize> map;
	map.reserve(count);
	for(usize i = 0; i != count / 1000 + 10; ++i) {
		map.insert({i * 1000, i});
	}

	usize a = 0;
	for(usize i = 0; i != 50; ++i) {
		for(const auto& [k, v] : map) {
			a += k * i + v;
		}
	}
	map.insert({a, a});

	return map;
}

template<template<typename...> typename Map>
static auto bench_fill_iter_erased_50(usize count = 1000 * bench_count_mul) {
	Map<usize, usize> map;

	for(usize i = 0; i != count; ++i) {
		map.insert({i, i});
	}

	for(usize i = 0; i != count; ++i) {
		if(i % 1000 == 0) {
			map.erase(map.find(i));
		}
	}

	usize a = 0;
	for(usize i = 0; i != 50; ++i) {
		for(const auto& [k, v] : map) {
			a += k * i + v;
		}
	}
	map.insert({a, a});

	return map;
}


template<template<typename...> typename Map>
static auto bench_fill_erase_all(usize count = 1000 * bench_count_mul) {
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
static auto bench_fill_erase_refill(usize count = 1000 * bench_count_mul) {
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
static auto bench_fill_find_all_50_50(usize count = 1000 * bench_count_mul) {
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

#if 0
template<template<typename...> typename Map, typename Hasher = std::hash<usize>>
static auto bench_fill_contains_50_50(usize count = 10000 * bench_count_mul) {
	Map<usize, usize, Hasher> map;

	math::FastRandom rng;
	std::uniform_int_distribution<usize> dist(0, count * 2);

	for(usize i = 0; i != count; ++i) {
		map.insert({dist(rng), i});
	}

	usize sum = 0;
	for(usize i = 0; i != count * 2; ++i) {
		if(const auto& it = map.contains(i)) {
			sum += i;
		}
	}

	map.insert({sum, sum});
	return map;
}
#endif

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_degen(usize count = 1000 * bench_count_mul) {
	Map<usize, usize> map;

	for(usize i = 0; i != count; ++i) {
		map.insert({i * 2, i});
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
static auto bench_fill_find_all_50_50_huge(usize count = 10000 * bench_count_mul) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_medium(usize count = 10 * bench_count_mul) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_tiny(usize count = 100) {
	return bench_fill_find_all_50_50<Map>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_find_all_50_50_badhash_8(usize count = 10 * bench_count_mul) {
	return bench_fill_find_all_50_50<Map, BadHash<8>>(count);
}

template<template<typename...> typename Map>
static auto bench_fill_long_string(usize count = 1000 * bench_count_mul, usize extra_len = 0) {
	Map<core::String, usize> map;

	core::String long_str;
	while(!long_str.is_long()) {
		long_str.push_back('a');
	}

	for(usize i = 0; i != extra_len; ++i) {
		long_str.push_back('l');
	}

	core::String key;
	for(usize i = 0; i != count; ++i) {
		key.make_empty();
		fmt_into(key, "%_%", i, long_str);
		map.insert({key, i});
	}

	return map;
}

template<template<typename...> typename Map>
static auto bench_fill_very_long_string(usize count = 200 * bench_count_mul) {
	return bench_fill_long_string<Map>(count, 1000);
}


template<template<typename...> typename Map>
static auto bench_fill_string_iter_50(usize count = 1000 * bench_count_mul) {
	Map<core::String, usize> map = bench_fill_long_string<Map>(count, 10);

	usize a = 0;
	for(usize i = 0; i != 25; ++i) {
		for(const auto& [k, v] : map) {
			a += k.size() * i;
		}
	}
	map.insert({fmt("%", a), a});

	a = 0;
	for(usize i = 0; i != 25; ++i) {
		for(const auto& [k, v] : map) {
			a += v * i;
		}
	}
	map.insert({fmt("%", a), a});

	return map;
}

template<template<typename...> typename Map>
static auto bench_fill_string_find_all_50_50(usize count = 1000 * bench_count_mul) {
	Map<core::String, usize> map;

	core::String long_str;
	while(long_str.size() < core::String::max_short_size + 10) {
		long_str.push_back('a');
	}

	core::String key;
	for(usize i = 0; i != count; ++i) {
		key.make_empty();
		fmt_into(key, "%_%", i, long_str);
		map.insert({key, i});
	}


	usize sum = 0;
	for(usize i = 0; i != count * 2; ++i) {
		key.make_empty();
		fmt_into(key, "%_%", i, long_str);
		if(const auto& it = map.find(key); it != map.end()) {
			sum += it->first.size() + it->second;
		}
	}
	map.insert({fmt("%", sum), sum});

	return map;
}



using result_type = core::Vector<std::tuple<const char*, double, usize>>;

template<template<typename...> typename Map>
result_type bench_implementation() {
	result_type results;
	core::DebugTimer _(ct_type_name<Map<int, int>>());

	const double min_time = 1.0;

#define BENCH_ONE(func)																	\
	do {																				\
		try {																			\
			log_msg("Running " #func, Log::Perf);										\
			core::Chrono chrono;														\
			for(usize count = 1; true; ++count) {										\
				const usize res = func<Map>().size();									\
				const auto elapsed = chrono.elapsed();									\
				if(elapsed.to_secs() >= min_time) {										\
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
	BENCH_ONE(bench_fill_iter_50_medium);
	BENCH_ONE(bench_fill_iter_50_tiny);
	BENCH_ONE(bench_fill_iter_empty_50);
	BENCH_ONE(bench_fill_iter_erased_50);
	BENCH_ONE(bench_fill_erase_all);
	BENCH_ONE(bench_fill_erase_refill);
	BENCH_ONE(bench_fill_find_all_50_50_degen);
	BENCH_ONE(bench_fill_find_all_50_50_huge);
	BENCH_ONE(bench_fill_find_all_50_50);
	BENCH_ONE(bench_fill_find_all_50_50_medium);
	BENCH_ONE(bench_fill_find_all_50_50_tiny);
	BENCH_ONE(bench_fill_find_all_50_50_badhash_8);
	BENCH_ONE(bench_fill_long_string);
	BENCH_ONE(bench_fill_very_long_string);
	BENCH_ONE(bench_fill_string_iter_50);
	BENCH_ONE(bench_fill_string_find_all_50_50);

	//BENCH_ONE(bench_fill_contains_50_50);

#undef BENCH_ONE

	return results;
}

template<typename K, typename V, typename H = std::hash<K>>
struct ExternalBitsMapStore : core::ExternalBitsDenseMap<K, V, H, true> {};

template<typename K, typename V, typename H = std::hash<K>>
struct ExternalBitsMap : core::ExternalBitsDenseMap<K, V, H, false> {};

int main() {
	y::test::run_tests();

	core::Vector<std::pair<const char*, result_type>> results;
	log_msg("Benching...");
	results.emplace_back("ExternalBitsMap", bench_implementation<ExternalBitsMap>());
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

			line = line.sub_str(0, 60);
			while(line.size() < 60) {
				line += " ";
			}

			core::Vector<double> times;
			for(const auto& i : results) {
				times << std::get<1>(i.second[r]);
			}
			std::sort(times.begin(), times.end());

			const double res_time = std::get<1>(res);
			if(res_time <= times[1] && times[1] * 0.95 < times[0]) {
				line += "~";
			}
			if(res_time == times.last() && res_time > times[0] * 2.0) {
				line += "!";
			}

			if(times[0] == res_time) {
				line += "*";
				if(times[1] * 0.5 > res_time) {
					line += "*";
				}
			} else {
				while(line.size() < 62) {
					line += " ";
				}
				const double mul = res_time / times[0];
				fmt_into(line, " %.%x", usize(mul), usize(mul * 10) % 10);
			}
			log_msg(line, Log::Perf);
		}
	}

	return 0;
}

#else

int main() {

	const bool ok = test::run_tests();

	if(ok) {
		log_msg("All tests OK\n");
	} else {
		log_msg("Tests failed\n", Log::Error);
	}

	return ok ? 0 : 1;
}

#endif


