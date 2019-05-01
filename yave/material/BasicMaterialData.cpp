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


BasicMaterialData::BasicMaterialData(std::array<AssetPtr<Texture>, texture_count>&& textures) :
		_textures(std::move(textures)) {
}

core::Result<BasicMaterialData> BasicMaterialData::load(io::ReaderRef reader, AssetLoader& loader) noexcept {
	try {
		BasicMaterialHeader().deserialize(reader);
		BasicMaterialData data;
		for(auto& tex : data._textures) {
			auto id = serde::deserialized<AssetId>(reader);
			if(id != AssetId::invalid_id()) {
				tex = std::move(loader.load<Texture>(id).or_throw("Unable to load texture."));
			}
		}
		return core::Ok(data);
	} catch(...) {
	}
	return core::Err();
}

const std::array<AssetPtr<Texture>, BasicMaterialData::texture_count>& BasicMaterialData::textures() const {
	return  _textures;
}

std::array<AssetId, BasicMaterialData::texture_count> BasicMaterialData::texture_ids() const {
	std::array<AssetId, texture_count> ids;
	std::transform(_textures.begin(), _textures.end(), ids.begin(), [](const auto& tex) { return tex.id(); });
	return ids;
}

}
