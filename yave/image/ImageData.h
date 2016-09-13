/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_IMAGE_IMAGEDATA_H
#define YAVE_IMAGE_IMAGEDATA_H

#include <yave/yave.h>

#include <y/io/Ref.h>
#include <y/math/Vec.h>

namespace yave {

class ImageData : NonCopyable {

	public:
		ImageData();
		~ImageData();

		ImageData(ImageData&& other);
		ImageData& operator=(ImageData&& other);

		usize byte_size() const;
		const math::Vec2ui& size() const;

		const u8* get_raw_pixel(const math::Vec2ui& pos);
		const u8* get_raw_data() const;

		static ImageData from_file(io::ReaderRef reader);

	private:
		void swap(ImageData& other);

		math::Vec2ui _size;
		usize _bpp;
		u8* _data;
};

}

#endif // YAVE_IMAGE_IMAGEDATA_H
