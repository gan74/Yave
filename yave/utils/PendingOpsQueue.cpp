/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "PendingOpsQueue.h"

namespace yave {

static bool is_done(const std::future<void>& future) {
     return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

PendingOpsQueue::PendingOpsQueue() {
}

PendingOpsQueue::~PendingOpsQueue() {
    y_profile_zone("waiting for pending ops");
    auto lock = y_profile_unique_lock(_lock);
    _pending_ops.clear();
}


void PendingOpsQueue::garbage_collect() {
    auto lock = y_profile_unique_lock(_lock);

    auto pending = core::vector_with_capacity<std::future<void>>(_pending_ops.size());
    for(auto& future : _pending_ops) {
        if(is_done(future)) {
            if(future.valid()) {
                future.get();
            }
        } else {
            pending.emplace_back(std::move(future));
        }
    }
    _pending_ops.swap(pending);
}

void PendingOpsQueue::push(std::future<void> future) {
    if(is_done(future)) {
        if(future.valid()) {
            future.get();
        }
        return;
    }

    auto lock = y_profile_unique_lock(_lock);
    Y_TODO(clean _pending_ops)
    _pending_ops.emplace_back(std::move(future));
}

}

