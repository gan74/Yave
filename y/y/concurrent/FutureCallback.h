/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_CONCURRENT_FUTURECALLBACK_H
#define Y_CONCURRENT_FUTURECALLBACK_H

#include <y/utils.h>

#include <future>

namespace y {
namespace concurrent {

template<typename R>
class FutureCallback {
	struct FutureBase {
		virtual ~FutureBase() = default;
		virtual R get() = 0;
		virtual bool is_done() const = 0;
	};

	template<typename T, typename F>
	struct Future : FutureBase {
		public:
			Future(std::future<T> future, F func) : _future(std::move(future)), _func(std::move(func)) {
			}

			R get() override {
				return _func(_future.get());
			}

			bool is_done() const override {
				return _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
			}

		private:
			std::future<T> _future;
			F _func;
	};

	public:
	    template<typename T, typename F>
		FutureCallback(std::future<T>&& future, F&& func) : _future(std::make_unique<Future<T, F>>(std::move(future), y_fwd(func))) {
		}

		R get() {
			return _future->get();
		}

		bool is_done() const {
			return _future->is_done();
		}


	private:
		std::unique_ptr<FutureBase> _future;
};

}
}

#endif // Y_CONCURRENT_FUTURECALLBACK_H
