/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef Y_IO2_IO_H
#define Y_IO2_IO_H

#include <memory>

#include <y/core/Vector.h>
#include <y/core/Result.h>

namespace y {
namespace io2 {

class File;
class Buffer;

using ReadUpToResult = core::Result<usize, usize>;
using ReadResult = core::Result<void, usize>;
using WriteResult = core::Result<void, usize>;
using FlushResult = core::Result<void>;

class Reader : NonCopyable {
    public:
        Reader() = default;
        virtual ~Reader() = default;

        virtual bool at_end() const = 0;
        virtual usize remaining() const = 0;

        virtual ReadResult read(void* data, usize bytes) = 0;
        virtual ReadUpToResult read_up_to(void* data, usize max_bytes) = 0;
        virtual ReadUpToResult read_all(core::Vector<u8>& data) = 0;

        virtual void seek(usize byte) = 0;
        virtual usize tell() const = 0;

        template<typename T>
        ReadResult read_one(T& t) {
            static_assert(std::is_trivially_copyable_v<T>);
            return read(&t, sizeof(T));
        }

        template<typename T>
        core::Result<std::remove_cvref_t<T>, usize> read_one() {
            std::remove_cvref_t<T> t;
            if(const auto r = read_one(t); !r) {
                return core::Err(r.error());
            }
            return core::Ok(std::move(t));
        }


        template<typename T>
        ReadResult read_array(T* data, usize count) {
            static_assert(std::is_trivially_copyable_v<T>);
            return read(static_cast<void*>(data), sizeof(T) * count);
        }
};

class Writer : NonCopyable {
    public:
        Writer() = default;
        virtual ~Writer() = default;

        virtual void seek(usize byte) = 0;
        virtual usize tell() const = 0;

        virtual FlushResult flush() = 0;
        virtual WriteResult write(const void* data, usize bytes) = 0;

        template<typename T>
        WriteResult write_one(const T& t) {
            static_assert(std::is_trivially_copyable_v<T>);
            return write(&t, sizeof(T));
        }

        template<typename T>
        WriteResult write_array(const T* data, usize count) {
            static_assert(std::is_trivially_copyable_v<T>);
            return write(static_cast<const void*>(data), sizeof(T) * count);
        }
};

using ReaderPtr = std::unique_ptr<Reader>;
using WriterPtr = std::unique_ptr<Writer>;

}
}

#endif // Y_IO2_IO_H

