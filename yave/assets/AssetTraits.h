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
#ifndef YAVE_ASSETS_ASSETTRAITS_H
#define YAVE_ASSETS_ASSETTRAITS_H

#include "AssetType.h"

#include <y/core/Result.h>

namespace yave {

class AssetLoader;

namespace detail {
DevicePtr device_from_loader(AssetLoader& loader);
}

template<typename T>
struct AssetTraits {
	static constexpr bool is_asset = false;
};

#define YAVE_FILL_ASSET_TRAITS(Type, LoadFrom, TypeEnum, UseNew)						\
	static constexpr bool is_asset = true;												\
	static constexpr AssetType type = TypeEnum;											\
	using load_from = LoadFrom;															\
	using Result = core::Result<Type>;													\
	static Result load_asset(io::ReaderRef reader, AssetLoader& loader) noexcept {		\
		try {																			\
			auto data = serde::deserialized<load_from>(reader);							\
			return core::Ok(Type(detail::device_from_loader(loader), data));			\
		} catch(...) {																	\
		}																				\
		return core::Err();																\
	}


#define YAVE_DECLARE_ASSET_TRAITS(Type, LoadFrom, TypeEnum)								\
	template<>																			\
	struct AssetTraits<Type> {															\
		YAVE_FILL_ASSET_TRAITS(Type, LoadFrom, TypeEnum, false)							\
	}


}

#endif // YAVE_ASSETS_ASSETTRAITS_H
