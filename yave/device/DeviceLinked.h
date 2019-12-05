/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef YAVE_DEVICE_DEVICELINKED_H
#define YAVE_DEVICE_DEVICELINKED_H

#include <yave/yave.h>

namespace yave {

template<typename T>
class SwapMove {
	public:
		SwapMove() = default;

		SwapMove(const SwapMove& other) = default;
		SwapMove& operator=(const SwapMove& other) = default;

		SwapMove(SwapMove&& other) {
			std::swap(_t, other._t);
		}

		SwapMove(T&& other) : _t(std::move(other)) {
		}

		SwapMove(const T& other) : _t(other) {
		}



		SwapMove& operator=(SwapMove&& other) {
			std::swap(_t, other._t);
			return *this;
		}

		SwapMove& operator=(T&& other) {
			_t = std::move(other);
			return *this;
		}

		SwapMove& operator=(const T& other) {
			_t = other;
			return *this;
		}


		void swap(SwapMove& other) {
			std::swap(_t, other._t);
		}


		operator T&() {
			return _t;
		}

		operator const T&() const {
			return _t;
		}



		T& get() {
			return _t;
		}

		const T& get() const {
			return _t;
		}

	private:
		T _t = {};
};

class DeviceLinked {
	public:
		~DeviceLinked() = default;
		
		DevicePtr device() const;
		bool is_null() const;

		template<typename T>
		void destroy(T&& t) const;

		template<typename T>
		void destroy_immediate(T&& t) const;

	protected:
		// Only default constructor should not link any device: explicitly passing nullptr to DeviceLinked is an error
		DeviceLinked();
		DeviceLinked(DevicePtr dev);
		DeviceLinked(ThreadDevicePtr dev);

		DeviceLinked(const DeviceLinked&) = default;
		DeviceLinked& operator=(const DeviceLinked&) = default;

		DeviceLinked(DeviceLinked&& other);
		DeviceLinked& operator=(DeviceLinked&& other);
		

		void swap(DeviceLinked& other);

	private:
		DevicePtr _device = nullptr;
};

class ThreadDeviceLinked {
	public:
		~ThreadDeviceLinked() = default;
		
		ThreadDevicePtr thread_device() const;
		DevicePtr device() const;

		template<typename T>
		void destroy(T&& t) const;

		template<typename T>
		void destroy_immediate(T&& t) const;

	protected:
		ThreadDeviceLinked();
		ThreadDeviceLinked(ThreadDevicePtr dev);

		ThreadDeviceLinked(const ThreadDeviceLinked&) = default;
		ThreadDeviceLinked& operator=(const ThreadDeviceLinked&) = default;

		ThreadDeviceLinked(ThreadDeviceLinked&& other);
		ThreadDeviceLinked& operator=(ThreadDeviceLinked&& other);

		void swap(ThreadDeviceLinked& other);

	private:
		ThreadDevicePtr _device = nullptr;
};

}

#endif // YAVE_DEVICE_DEVICELINKED_H
