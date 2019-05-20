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
#ifndef YAVE_ASSETS_ASSETSTORE_H
#define YAVE_ASSETS_ASSETSTORE_H

#include "AssetPtr.h"
#include "AssetType.h"

namespace yave {

class FileSystemModel;

class AssetStore : NonCopyable {

	public:
		enum class ErrorType {
			UnknownID,
			AlreadyExistingID,

			InvalidName,

			FilesytemError,

			UnsupportedOperation,
			Unknown,
		};

		template<typename T = void>
		using Result = core::Result<T, ErrorType>;



		AssetStore();
		virtual ~AssetStore();

		virtual const FileSystemModel* filesystem() const;

		virtual Result<AssetId> import(io2::Reader& data, std::string_view dst_name) = 0;
		virtual Result<> write(AssetId id, io2::Reader& data);

		virtual Result<AssetId> id(std::string_view name) const = 0;
		virtual Result<core::String> name(AssetId id) const = 0;

		virtual Result<io2::ReaderPtr> data(AssetId id) const = 0;

		virtual Result<> remove(AssetId id);
		virtual Result<> rename(AssetId id, std::string_view new_name);

		virtual Result<> remove(std::string_view name);
		virtual Result<> rename(std::string_view from, std::string_view to);

		virtual Result<AssetType> asset_type(AssetId id) const;
};

}

#endif // YAVE_ASSETS_ASSETSTORE_H
