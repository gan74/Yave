/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_FONT_FONT_H
#define YAVE_FONT_FONT_H

#include "FontData.h"
#include <yave/images/Image.h>
#include <yave/meshes/StaticMesh.h>

namespace yave {

class Font : NonCopyable {
	public:
		struct CharData {
			CharData(const FontData::Char& c) : uv(c.uv), size(c.size) {
			}

			math::Vec2 uv;
			math::Vec2 size;
		};


		Font() = default;
		Font(DevicePtr dptr, const FontData& data);

		Font(Font&& other);
		Font& operator=(Font&& other);


		CharData char_data(u32 c) const;

		const Texture& char_atlas() const;
		const StaticMesh& quad_mesh() const;

		DevicePtr device() const;

	private:
		void swap(Font& other);

		Texture _font_atlas;
		StaticMesh _quad;

		std::unordered_map<u32, FontData::Char> _chars;


};

}

#endif // YAVE_FONT_FONT_H
