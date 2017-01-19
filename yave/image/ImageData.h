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
#ifndef YAVE_IMAGE_IMAGEDATA_H
#define YAVE_IMAGE_IMAGEDATA_H

#include <yave/yave.h>

#include <y/io/Ref.h>
#include <y/math/Vec.h>

namespace yave {

class ImageData : NonCopyable {

	public:
		ImageData() = default;

		ImageData(ImageData&& other);
		ImageData& operator=(ImageData&& other);

		usize byte_size() const;
		usize all_mip_bytes_size() const;

		const math::Vec2ui& size() const;

		usize bpp() const;
		usize mipmaps() const;
		math::Vec2ui mip_size(usize lvl) const;

		const u8* data() const;

		static ImageData from_file(io::ReaderRef reader);

	private:
		void swap(ImageData& other);

		math::Vec2ui _size;
		usize _bpp = 0;
		usize _mips = 1;
		core::Unique<u8[]> _data;
};

}

#endif // YAVE_IMAGE_IMAGEDATA_H
