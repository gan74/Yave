/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_DEVICELINKED_H
#define YAVE_DEVICELINKED_H

#include "yave.h"

namespace yave {

class DeviceLinked {
	public:
		// Only default constructor should not link any device: explicitly passing nullptr to DeviceLinked is an error
		DeviceLinked() : _device(nullptr) {
			// for putting breakpoints
		}

		DeviceLinked(DevicePtr dev) : _device(dev) {
			if(!dev) {
				fatal("Null device");
			}
		}

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
		void swap(DeviceLinked& other) {
			std::swap(_device, other._device);
		}

	private:
		DevicePtr _device;

};

}

#endif // YAVE_DEVICELINKED_H
