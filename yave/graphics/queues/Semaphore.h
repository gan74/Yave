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
#ifndef YAVE_GRAPHICS_QUEUES_SEMAPHORE_H
#define YAVE_GRAPHICS_QUEUES_SEMAPHORE_H

#include <yave/device/DeviceLinked.h>
#include <yave/graphics/vk/vk.h>

#include <memory>

namespace yave {

template<typename T>
class BoxSemaphore;

class Semaphore {
    class Shared : NonMovable, public DeviceLinked {
        public:
            Shared() = default;
            Shared(DevicePtr dptr);

            ~Shared();

            VkSemaphore vk_semaphore() const;

        private:
            VkSemaphore _semaphore = {};
    };


    public:
        Semaphore() = default;

        DevicePtr device() const;
        bool is_null() const;

        VkSemaphore vk_semaphore() const;

        bool operator==(const Semaphore& other) const;

        template<typename T>
        BoxSemaphore<T> box(T&& t) const;

    protected:
        friend class Queue;

        Semaphore(DevicePtr dptr);

    private:
        std::shared_ptr<Shared> _semaphore;
};



template<typename T>
class BoxSemaphore : private Semaphore {
    public:
        BoxSemaphore(DevicePtr dptr, T&& obj) : Semaphore(dptr), _boxed(std::move(obj)) {
        }

        BoxSemaphore(const Semaphore& sem, T&& obj) : Semaphore(sem), _boxed(std::move(obj)) {
        }

        BoxSemaphore(T&& obj) : _boxed(std::move(obj)) {
        }


    private:
        friend class CmdBufferRecorder;

        T _boxed;
};


template<typename T>
BoxSemaphore<T> Semaphore::box(T&& t) const {
    return BoxSemaphore(*this, y_fwd(t));
}

}

#endif // YAVE_GRAPHICS_QUEUES_SEMAPHORE_H

