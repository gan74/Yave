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

#include "FontData.h"

namespace yave {

FontData::FontData(FontData&& other) {
	std::swap(_font_atlas, other._font_atlas);
	std::swap(_chars, other._chars);
}

const ImageData& FontData::atlas_data() const {
	return _font_atlas;
}

FontData FontData::from_file(io::ReaderRef reader) {
	Y_LOG_PERF("Loading");
	const char* err_msg = "Unable to read font.";

	FontData font;
	font._font_atlas = ImageData::from_file(reader);

	struct Header {
		u32 magic;
		u32 type;
		u32 version;
		u32 char_count;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::font_file_type &&
				   version == 1 &&
				   char_count > 0;
		}
	};

	Header header = reader->read_one<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	core::Unique<Char[]> chars = new Char[header.char_count];
	reader->read(chars, header.char_count * sizeof(Char)).expected(err_msg);

	/*for(u32 i = 0; i != header.char_count; ++i) {
		font._chars[chars[i].utf32] = chars[i];
		log_msg(core::str(chars[i].utf32) + " " + chars[i].uv.x() + " " + chars[i].uv.y() + " " + chars[i].size.x() + " " + chars[i].size.y());
	}*/

	return font;
}

}
