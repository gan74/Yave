/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

class DeviceLinked {
	public:
		DevicePtr device() const {
			return _device;
		}

		template<typename T>
		void destroy(T t) const; /* {
			if(_device) {
				_device->destroy(t);
			}
		}*/

	protected:
		// Only default constructor should not link any device: explicitly passing nullptr to DeviceLinked is an error
		DeviceLinked() : _device(nullptr) {
			// for putting breakpoints
		}

		DeviceLinked(DevicePtr dev) : _device(dev) {
			if(!dev) {
				fatal("Null device.");
			}
		}

		void swap(DeviceLinked& other) {
			std::swap(_device, other._device);
		}

	private:
		DevicePtr _device;

};

}

#endif // YAVE_DEVICE_DEVICELINKED_H
