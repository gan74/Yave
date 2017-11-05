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

#include "Font.h"

namespace yave {

static MeshData create_quad_data() {
	return MeshData::from_parts(core::Vector<Vertex>{{{0.0f, 0.0f, 0.0f}, {}, {}, {0.0f, 0.0f}},
													 {{0.0f, 1.0f, 0.0f}, {}, {}, {0.0f, 1.0f}},
													 {{1.0f, 1.0f, 0.0f}, {}, {}, {1.0f, 1.0f}},
													 {{1.0f, 0.0f, 0.0f}, {}, {}, {1.0f, 0.0f}}},
								core::Vector<IndexedTriangle>{{0, 1, 2}, {2, 3, 0}});
}


Font::Font(DevicePtr dptr, const FontData& data) :
		_font_atlas(dptr, data.atlas_data()),
		_quad(dptr, create_quad_data()),
		_chars(data._chars) {
}

Font::Font(Font&& other) {
	swap(other);
}

Font& Font::operator=(Font&& other) {
	swap(other);
	return *this;
}

void Font::swap(Font& other) {
	std::swap(_font_atlas, other._font_atlas);
	std::swap(_quad, other._quad);
	std::swap(_chars, other._chars);
}

DevicePtr Font::device() const {
	return _font_atlas.device();
}

const Texture& Font::char_atlas() const {
	return _font_atlas;
}

const StaticMesh& Font::quad_mesh() const {
	return _quad;
}

Font::CharData Font::char_data(u32 c) const {
	auto it = _chars.find(c);
	return (it == _chars.end() ? _chars.begin() : it)->second;
}

}
