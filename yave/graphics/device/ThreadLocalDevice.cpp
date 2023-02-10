/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "ThreadLocalDevice.h"

#include <yave/graphics/commands/CmdBufferPool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {
namespace device {
extern std::atomic<u64> active_pools;
}

ThreadLocalDevice::ThreadLocalDevice() :
        _disposable_cmd_pool(this),
        _lifetime_manager(this) {

    ++device::active_pools;
}

ThreadLocalDevice::~ThreadLocalDevice() {
    --device::active_pools;
}

CmdBufferRecorder ThreadLocalDevice::create_disposable_cmd_buffer() const {
    return _disposable_cmd_pool.create_buffer();
}

ThreadLocalLifetimeManager& ThreadLocalDevice::lifetime_manager() const {
    return _lifetime_manager;
}

}

