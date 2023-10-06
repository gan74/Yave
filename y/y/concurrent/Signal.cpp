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

#include "Signal.h"

namespace y {
namespace concurrent {

Subscription::~Subscription() {
    disconnect();
}

Subscription::Subscription(Subscription&& other) {
    swap(other);
}

Subscription& Subscription::operator=(Subscription&& other) {
    swap(other);
    return *this;
}

bool Subscription::is_connected() const {
    return !_data.expired();
}

void Subscription::swap(Subscription& other) {
    std::swap(_data, other._data);
    std::swap(_index, other._index);
}

void Subscription::disconnect() {
    if(auto ptr = _data.lock()) {
        ptr->disconnect(_index);
    }
    _data.reset();
}

void Subscription::detach() && {
    _data.reset();
}

Subscription::Subscription(std::weak_ptr<detail::SignalDataBase> data, u32 index) : _data(std::move(data)), _index(index) {
}

}
}

