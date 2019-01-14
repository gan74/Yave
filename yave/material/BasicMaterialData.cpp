/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
	y_serde(fs::magic_number, fs::material_file_type, u32(1))
};

BasicMaterialData::BasicMaterialData(DevicePtr dptr) : DeviceLinked(dptr) {
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

MaterialData BasicMaterialData::create_material_data() const {
	auto data = MaterialData()
				.set_frag_data(device()->default_resources()[DefaultResources::BasicFrag])
				.set_vert_data(device()->default_resources()[DefaultResources::BasicVert]);

	for(usize i = 0; i != texture_count; ++i) {
		data.keep_alive(_textures[i]);
		data.add_binding(Binding(*_textures[i]));
	}

	return data;
}


}
