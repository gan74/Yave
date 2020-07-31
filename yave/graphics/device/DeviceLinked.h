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
#ifndef YAVE_DEVICE_DEVICELINKED_H
#define YAVE_DEVICE_DEVICELINKED_H

#include <yave/graphics/utils.h>

namespace yave {

class DeviceLinked : NonCopyable {
    public:
        ~DeviceLinked() = default;

        DevicePtr device() const;
        bool is_null() const;

        template<typename T>
        void destroy(T&& t) const {
            device_destroy(_device, y_fwd(t));
        }

    protected:
        // Only default constructor should not link any device: explicitly passing nullptr to DeviceLinked is an error
        DeviceLinked();
        DeviceLinked(DevicePtr dev);
        DeviceLinked(ThreadDevicePtr dev);

        DeviceLinked(DeviceLinked&& other);
        DeviceLinked& operator=(DeviceLinked&& other);


        void swap(DeviceLinked& other);

    private:
        DevicePtr _device = nullptr;
};

class ThreadDeviceLinked : NonCopyable {
    public:
        ~ThreadDeviceLinked() = default;

        ThreadDevicePtr thread_device() const;
        DevicePtr device() const;

        template<typename T>
        void destroy(T&& t) const {
            device_destroy(_device, y_fwd(t));
        }

    protected:
        ThreadDeviceLinked();
        ThreadDeviceLinked(ThreadDevicePtr dev);

        ThreadDeviceLinked(ThreadDeviceLinked&& other);
        ThreadDeviceLinked& operator=(ThreadDeviceLinked&& other);

        void swap(ThreadDeviceLinked& other);

    private:
        ThreadDevicePtr _device = nullptr;
};
}

#endif // YAVE_DEVICE_DEVICELINKED_H

