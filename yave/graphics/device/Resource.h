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
#ifndef YAVE_DEVICE_RESOURCE_H
#define YAVE_DEVICE_RESOURCE_H

#include "ResourceType.h"

namespace yave {

#define YAVE_GENERATE_DESTROY(T) void device_destroy(DevicePtr dptr, T t);
YAVE_GRAPHIC_RESOURCE_TYPES(YAVE_GENERATE_DESTROY)
#undef YAVE_GENERATE_DESTROY

template<typename T>
class Resource {
    public:
        Resource() = default;

        Resource(Resource&& other) {
            std::swap(_t, other._t);
        }

        Resource(T&& other) : _t(std::move(other)) {
        }

        Resource(const T& other) : _t(other) {
        }

        inline void destroy(DevicePtr dptr) {
            device_destroy(dptr, std::move(_t));
        }

        Resource& operator=(Resource&& other) {
            std::swap(_t, other._t);
            return *this;
        }

        Resource& operator=(T&& other) {
            _t = std::move(other);
            return *this;
        }

        void swap(Resource& other) {
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

}

#endif // YAVE_DEVICE_RESOURCE_H

