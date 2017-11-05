/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_DEVICE_THREADLOCALDEVICEDATA_H
#define YAVE_DEVICE_THREADLOCALDEVICEDATA_H

#include <yave/vk/vk.h>
#include <yave/bindings/DescriptorSetLayoutPool.h>
#include <yave/commands/pool/CmdBufferPool.h>

#include "DeviceLinked.h"

namespace yave {

class ThreadLocalDeviceData : NonCopyable, public DeviceLinked {
	public:
		~ThreadLocalDeviceData();

		vk::Device vk_device() const;

	private:
		friend class Device;

		NotOwner<vk::Device> _device;

		ThreadLocalDeviceData(DevicePtr dptr);
};

}

#endif // YAVE_DEVICE_THREADLOCALDEVICEDATA_H
