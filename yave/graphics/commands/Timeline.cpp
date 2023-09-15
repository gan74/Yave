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

#include "Timeline.h"

namespace yave {


TimelineFence::TimelineFence(u64 value, const Timeline* parent) : _value(value), _parent(parent) {
}

void TimelineFence::wait() const {
    y_debug_assert(_parent);
    _parent->wait(*this);
}

bool TimelineFence::is_ready() const {
    y_debug_assert(_parent);
    return _parent->is_ready(*this);
}

u64 TimelineFence::value() const {
    y_debug_assert(_parent);
    return _value;
}

bool TimelineFence::is_valid() const {
    return _parent;
}

bool TimelineFence::operator==(const TimelineFence& other) const {
    y_debug_assert(_parent == other._parent);
    return _value == other._value;
}

bool TimelineFence::operator!=(const TimelineFence& other) const {
    y_debug_assert(_parent == other._parent);
    return _value != other._value;
}

bool TimelineFence::operator<(const TimelineFence& other) const {
    y_debug_assert(_parent == other._parent);
    return _value < other._value;
}

bool TimelineFence::operator<=(const TimelineFence& other) const {
    y_debug_assert(_parent == other._parent);
    return _value <= other._value;
}

// https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value
template<typename T>
static void update_maximum(std::atomic<T>& maximum_value, const T& value) noexcept {
    T prev_value = maximum_value;
    while(prev_value < value && !maximum_value.compare_exchange_weak(prev_value, value)) {
    }
}


static VkSemaphore create_timeline_semaphore() {
    VkSemaphoreTypeCreateInfo type_create_info = vk_struct();
    type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

    VkSemaphoreCreateInfo create_info = vk_struct();
    create_info.pNext = &type_create_info;

    VkSemaphore semaphore = {};
    vk_check(vkCreateSemaphore(vk_device(), &create_info, vk_allocation_callbacks(), &semaphore));

    /*VkSemaphoreSignalInfo signal_info = vk_struct();
    signal_info.semaphore = semaphore;
    signal_info.value = 1;
    vk_check(vkSignalSemaphore(vk_device(), &signal_info));*/

    return semaphore;
}



Timeline::Timeline() : _semaphore(create_timeline_semaphore()) {
}

Timeline::~Timeline() {
    wait();
    vkDestroySemaphore(vk_device(), _semaphore, vk_allocation_callbacks());
}

TimelineFence Timeline::advance_timeline() {
    return TimelineFence(++_value, this);
}

TimelineFence Timeline::current_timeline() const {
    return TimelineFence(_value, this);
}

TimelineFence Timeline::last_ready() const {
    u64 value = 0;
    vk_check(vkGetSemaphoreCounterValue(vk_device(), _semaphore, &value));
    update_maximum(_ready, value);
    return TimelineFence(value, this);
}

bool Timeline::is_ready(TimelineFence fence) const {
    y_debug_assert(fence._parent == this);

    /*if(fence._value <= _ready) {
        return true;
    }*/

    return fence <= last_ready();
}


void Timeline::wait(TimelineFence fence) const {
    y_profile();

    y_debug_assert(fence._parent == this);

    const u64 value = fence.value();
    /*if(value <= _ready) {
        return;
    }*/


    VkSemaphoreWaitInfo wait_info = vk_struct();
    {
        wait_info.pSemaphores = &_semaphore;
        wait_info.pValues = &value;
        wait_info.semaphoreCount = 1;
    }
    vk_check(vkWaitSemaphores(vk_device(), &wait_info, u64(-1)));
    update_maximum(_ready, value);
}

void Timeline::wait() const {
    wait(current_timeline());
}

VkSemaphore Timeline::vk_semaphore() const {
    return _semaphore;
}


}


