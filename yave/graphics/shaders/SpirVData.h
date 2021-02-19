/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_SHADERS_SPIRVDATA_H
#define YAVE_GRAPHICS_SHADERS_SPIRVDATA_H

#include <yave/yave.h>

#include <y/reflect/reflect.h>
#include <y/core/Vector.h>
#include <y/core/Span.h>

#include <y/io2/io.h>

namespace yave {

class SpirVData {

    public:
        SpirVData() = default;

        usize size() const;
        bool is_empty() const;

        const u32* data() const;

        Y_TODO(what is this?)
        static SpirVData deserialized(io2::Reader& reader);

    private:
        SpirVData(core::Span<u32> data);
        SpirVData(core::Span<u8> data);

        core::Vector<u32> _data;
};

}

#endif // YAVE_GRAPHICS_SHADERS_SPIRVDATA_H

