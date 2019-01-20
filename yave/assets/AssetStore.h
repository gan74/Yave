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

#include <y/core/String.h>
#include <y/io/Ref.h>

#include <yave/utils/FileSystemModel.h>

#include "AssetPtr.h"

namespace yave {

class AssetStore : NonCopyable {

	public:
		AssetStore();
		virtual ~AssetStore();

		virtual const FileSystemModel* filesystem() const;

		virtual AssetId import(io::ReaderRef data, std::string_view dst_name) = 0;

		virtual AssetId id(std::string_view name) const = 0;
		virtual io::ReaderRef data(AssetId id) const = 0;

		virtual void remove(AssetId id);
		virtual void rename(AssetId id, std::string_view new_name);

		virtual void remove(std::string_view name);
		virtual void rename(std::string_view from, std::string_view to);

		virtual void write(AssetId id, io::ReaderRef data);
};

}

#endif // YAVE_ASSETS_ASSETSTORE_H
