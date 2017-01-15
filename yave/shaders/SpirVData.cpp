/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

namespace yave {

SpirVData SpirVData::from_file(io::ReaderRef reader) {
	Y_TODO(optimise)
	core::Vector<u8> content;
	reader->read_all(content);
	core::Vector<u32> spriv32(content.size() / 4, 0);
	memcpy(spriv32.begin(), content.begin(), content.size());
	return SpirVData(spriv32);
}



SpirVData::SpirVData(const core::Vector<u32>& data) : _data(data) {
	if(data.is_empty()) {
		fatal("Invalid SPIR-V data.");
	}
}

usize SpirVData::size() const {
	return _data.size() * 4;
}

const u32* SpirVData::data() const {
	return _data.begin();
}

bool SpirVData::is_empty() const {
	return _data.is_empty();
}

}
