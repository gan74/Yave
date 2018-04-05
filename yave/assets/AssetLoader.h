/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_ASSETS_ASSETLOADER_H
#define YAVE_ASSETS_ASSETLOADER_H

#include <yave/device/DeviceLinked.h>
#include <y/core/String.h>
#include <y/io/File.h>

#include "AssetPtr.h"

#include <unordered_map>

namespace yave {

template<typename T>
struct AssetTraits {
	template<typename U>
	using has_load_from_t = typename U::load_from;

	static_assert(is_detected_v<has_load_from_t, T>, "Asset type should have ::load_from");

	// todo: try to deduce using TMP ?
	using load_from = typename T::load_from;
};

template<typename T>
class AssetLoader : public DeviceLinked {

	using traits = AssetTraits<T>;
	using load_from = typename traits::load_from;

	public:
		AssetLoader(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		core::Result<AssetPtr<T>> from_file(const core::String& filename) {
			auto it = _loaded.find(filename);
			if(it == _loaded.end()) {
				if(auto file = io::File::open(filename); file.is_ok()) {
					load_from data = load_from::from_file(file.unwrap());
					it = _loaded.insert({filename, AssetPtr<T>::make(device(), std::move(data))}).first;
				} else {
					return core::Err();
				}

			}
			return core::Ok(it->second);
		}


		usize size() const {
			return _loaded.size();
		}

		auto begin() const {
			return _loaded.begin();
		}

		auto end() const {
			return _loaded.end();
		}

	private:
		std::unordered_map<core::String, AssetPtr<T>> _loaded;
};

}

#endif // YAVE_ASSETS_ASSETLOADER_H
