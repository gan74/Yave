/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#include "SpirVData.h"

#include <y/io2/io.h>

namespace yave {

SpirVData::SpirVData(core::Span<u8> data) {
    if(data.size() % 4) {
        y_fatal("Invalid SPIR-V data.");
    }
    _data = core::Vector<u32>(data.size() / 4, 0);
    std::memcpy(_data.begin(), data.begin(), data.size());
}

SpirVData::SpirVData(core::Span<u32> data) : _data(data) {
    if(data.is_empty()) {
        y_fatal("Invalid SPIR-V data.");
    }
}

SpirVData SpirVData::deserialized(io2::Reader& reader) {
    core::Vector<u8> data;
    reader.read_all(data).unwrap();
    return SpirVData(data);
}

core::Span<u32> SpirVData::data() const {
    return _data;
}

bool SpirVData::is_empty() const {
    return _data.is_empty();
}

}

