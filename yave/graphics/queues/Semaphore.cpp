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

#include "Semaphore.h"

#include <yave/device/DeviceUtils.h>

namespace yave {

Semaphore::Shared::Shared(DevicePtr dptr) :
        DeviceLinked(dptr) {
    const VkSemaphoreCreateInfo create_info = vk_struct();
    vk_check(vkCreateSemaphore(vk_device(device()), &create_info, vk_allocation_callbacks(device()), &_semaphore));
}

Semaphore::Shared::~Shared() {
    destroy(_semaphore);
}

VkSemaphore Semaphore::Shared::vk_semaphore() const {
    return _semaphore;
}





Semaphore::Semaphore(DevicePtr dptr) : _semaphore(std::make_shared<Shared>(dptr)) {
}

DevicePtr Semaphore::device() const {
    return _semaphore ? _semaphore->device() : nullptr;
}

bool Semaphore::is_null() const {
    return !device();
}

VkSemaphore Semaphore::vk_semaphore() const {
    return _semaphore->vk_semaphore();
}

bool Semaphore::operator==(const Semaphore& other) const {
    return other._semaphore == _semaphore;
}

}

