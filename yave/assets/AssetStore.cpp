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

AssetStore::Result<> AssetStore::remove(AssetId id) {
	unused(id);
	return core::Err(ErrorType::UnsupportedOperation);
}

AssetStore::Result<> AssetStore::rename(AssetId id, std::string_view new_name) {
	unused(id, new_name);
	return core::Err(ErrorType::UnsupportedOperation);
}

const FileSystemModel* AssetStore::filesystem() const {
	return nullptr;
}

AssetStore::Result<> AssetStore::remove(std::string_view name) {
	if(auto i = id(name)) {
		 return remove(i.unwrap());
	}
	return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<> AssetStore::rename(std::string_view from, std::string_view to) {
	if(auto i = id(from)) {
		 return rename(i.unwrap(), to);
	}
	return core::Err(ErrorType::UnknownID);
}

AssetStore::Result<> AssetStore::write(AssetId id, io2::Reader& data) {
	unused(id, data);
	return core::Err(ErrorType::UnsupportedOperation);
}

AssetStore::Result<AssetType> AssetStore::asset_type(AssetId id) const {
	auto dat = data(id);
	if(dat) {
		auto& reader = *dat.unwrap();
		using magic_t = decltype(fs::magic_number);
		if(reader.read_one<magic_t>().unwrap_or(0) == fs::magic_number) {
			if(auto type = reader.read_one<AssetType>()) {
				return core::Ok(type.unwrap());
			}
		}
		return core::Err(ErrorType::FilesytemError);
	}
	return core::Err(dat.error());
}

}
