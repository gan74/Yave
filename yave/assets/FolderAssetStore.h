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
#ifndef YAVE_ASSETS_FOLDERASSETSTORE_H
#define YAVE_ASSETS_FOLDERASSETSTORE_H


#include <yave/utils/filesystem.h>
#include <y/serde/serde.h>

#include "AssetStore.h"

#include <unordered_map>
#include <mutex>

#ifndef YAVE_NO_STDFS

namespace yave {

class FolderAssetStore final : public AssetStore {

	struct Entry {
		core::String name;
		AssetId id;

		y_serde(id, name)
	};

	public:
		FolderAssetStore(std::string_view path = "./store");
		~FolderAssetStore();

		AssetId import_as(std::string_view src_name, std::string_view dst_name, ImportType import_type) override;
		AssetId id(std::string_view name) override;

		io::ReaderRef data(AssetId id) override;

	private:
		void write_index();
		void read_index();

		fs::path file_path(std::string_view name) const;

		void try_import(std::string_view src, std::string_view dst, ImportType import_type);

		std::mutex _lock;
		fs::path _path;
		core::String _index_file_path;


		// TODO optimize ?
		std::unordered_map<AssetId, Entry*> _from_id;
		std::unordered_map<core::String, std::unique_ptr<Entry>> _from_name;
};

}

#endif // YAVE_NO_STDFS

#endif // YAVE_ASSETS_FOLDERASSETSTORE_H
