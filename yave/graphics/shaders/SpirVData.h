/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include <yave/utils/serde.h>

namespace yave {

class SpirVData {

	public:
		SpirVData() = default;

		usize size() const;
		bool is_empty() const;

		const u32* data() const;

		static SpirVData deserialized(io2::Reader& reader);

	private:
		SpirVData(core::ArrayView<u32> data);
		SpirVData(core::ArrayView<u8> data);

		core::Vector<u32> _data;
};

}

#endif // YAVE_GRAPHICS_SHADERS_SPIRVDATA_H
