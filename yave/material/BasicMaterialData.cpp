/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "BasicMaterialData.h"

#include <y/io/File.h>

namespace yave {

struct BasicMaterialHeader {
	y_serde(fs::magic_number, AssetType::Material, u32(1))
};


BasicMaterialData::BasicMaterialData(std::array<AssetPtr<Texture>, texture_count>&& textures) :
		_textures(std::move(textures)) {
}

BasicMaterialData BasicMaterialData::deserialized(io::ReaderRef reader, AssetLoader<Texture>& texture_loader) {
	BasicMaterialHeader().deserialize(reader);
	BasicMaterialData data;
	for(auto& tex : data._textures) {
		tex = texture_loader.load(serde::deserialized<AssetId>(reader));
	}
	return data;
}

void BasicMaterialData::serialize(io::WriterRef writer) const {
	BasicMaterialHeader().serialize(writer);
	for(const auto& tex : _textures) {
		serde::serialize(writer, tex.id());
	}
}

core::ArrayView<AssetPtr<Texture>> BasicMaterialData::textures() const {
	return  _textures;
}

}
