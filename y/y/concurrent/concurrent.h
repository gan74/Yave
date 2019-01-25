/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_CONCURRENT_CONCURRENT_H
#define Y_CONCURRENT_CONCURRENT_H

#include "StaticThreadPool.h"

#include <future>

namespace y {
namespace concurrent {

StaticThreadPool& default_thread_pool();

namespace detail {

usize probable_block_count();
usize probable_block_count(usize size);

template<typename F>
void schedule_n(F&& f, usize n) {
	StaticThreadPool& pool = default_thread_pool();
	for(usize i = 0; i != n; ++i) {
		pool.schedule([&] { f(i); });
	}
	pool.process_until_empty();
}

}

template<typename F>
auto async(F&& func) {
	using ret_t = decltype(func());
	std::promise<ret_t> promise;
	std::future<ret_t> future = promise.get_future();
	default_thread_pool().schedule([p = std::move(promise), f = y_fwd(func)]() mutable {
			if constexpr(std::is_void_v<ret_t>) {
				f();
				p.set_value();
			} else {
				p.set_value(f());
			}
		});
	return future;
}


template<typename It, typename Func>
void parallel_indexed_block_for(It begin, It end, Func&& func) {
	usize size = end - begin;
	usize chunk = std::max(usize(1), size / (detail::probable_block_count(size) - 1));

	if(!size) {
		return;
	}

	usize chunk_count = size / chunk;
	chunk_count += chunk_count * chunk != size;

	detail::schedule_n([=, &func](usize i) {
		usize first = i * chunk;
		usize last = std::min(size, first + chunk);
		func(i, core::Range(begin + first, begin + last));
	}, chunk_count);
}



template<typename It, typename Func>
void parallel_block_for(It begin, It end, Func&& func) {
	parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		func(range);
	});
}

template<typename It, typename Func>
void parallel_for_each(It begin, It end, Func&& func) {
	return parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		for(auto&& e : range) {
			func(e);
		}
	});
}

template<typename It, typename Func>
void parallel_for(It begin, It end, Func&& func) {
	return parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		for(auto i = range.begin(); i != range.end(); ++i) {
			func(i);
		}
	});
}



template<template<typename...> typename C = core::Vector, typename It, typename Func>
auto parallel_block_collect(It begin, It end, Func&& func) {
	using T = decltype(func(std::declval<core::Range<It>>()));
	std::mutex mutex;

	C<T> col;
	try_reserve(col, probable_block_count(std::distance(begin, end)));

	parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		auto e = func(range);

		std::unique_lock lock(mutex);
		col.push_back(std::move(e));
	});

	return col;
}

template<template<typename...> typename C = core::Vector, typename It, typename Func>
auto parallel_collect(It begin, It end, Func&& func) {
	auto cols = parallel_block_collect(begin, end, func);

	usize size = 0;
	for(const auto& c : cols) {
		size += c.size();
	}

	C<std::remove_reference_t<decltype(*cols.first().begin())>> sum;
	try_reserve(sum, size);

	for(const auto& c : cols) {
		std::copy(c.begin(), c.end(), std::back_inserter(sum));
	}

	return sum;
}

}
}

#endif // Y_CONCURRENT_CONCURRENT_H
