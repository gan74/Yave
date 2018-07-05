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
#ifndef YAVE_ASSETS_ASSETSTORE_H
#define YAVE_ASSETS_ASSETSTORE_H

#include <y/core/String.h>
#include "AssetPtr.h"

#include <y/io/Ref.h>

namespace yave {

class AssetStore : NonCopyable {

	public:
		enum ImportType {
			Intern,
			Reference
		};

		AssetStore() {
		}

		virtual ~AssetStore() {
		}


		virtual core::Result<void> remove(AssetId id) {
			unused(id);
			return core::Err();
		}

		virtual core::Result<AssetId> import_as(std::string_view src_name, std::string_view dst_name, ImportType import_type) = 0;
		virtual core::Result<AssetId> id(std::string_view name) = 0;

		virtual core::Result<io::ReaderRef> data(AssetId id) = 0;

		/*virtual core::Result<io::ReaderRef> data(std::string_view name) {
			if(auto r = id(name); r.is_ok()) {
				return data(id.unwrap());
			}
			return core::Err();
		}*/

};

}

#endif // YAVE_ASSETS_ASSETSTORE_H
