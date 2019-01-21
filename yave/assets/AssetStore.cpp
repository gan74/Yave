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

#include "AssetStore.h"

#include <yave/utils/serde.h>

namespace yave {

AssetStore::AssetStore() {
}

AssetStore::~AssetStore() {
}

void AssetStore::remove(AssetId id) {
	unused(id);
	y_throw("Unsuported operation.");
}

void AssetStore::rename(AssetId id, std::string_view new_name) {
	unused(id, new_name);
	y_throw("Unsuported operation.");
}

const FileSystemModel* AssetStore::filesystem() const {
	return nullptr;
}

void AssetStore::remove(std::string_view name) {
	remove(id(name));
}

void AssetStore::rename(std::string_view from, std::string_view to) {
	rename(id(from), to);
}

void AssetStore::write(AssetId id, io::ReaderRef data) {
	unused(id, data);
	y_throw("Unsuported operation.");
}

AssetType AssetStore::asset_type(AssetId id) const {
	io::ReaderRef reader = data(id);
	u32 magic = reader->read_one<u32>();
	if(magic != fs::magic_number) {
		y_throw("Unknown file type.");
	}
	return reader->read_one<AssetType>();
}

}
