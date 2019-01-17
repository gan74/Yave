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

namespace yave {

template<typename T>
struct AssetTraits {
	private:
		template<typename U>
		using has_load_from_t = typename U::load_from;
		static_assert(is_detected_v<has_load_from_t, T>, "Asset type should have ::load_from");

		/*template<typename U>
		using has_type_t = decltype(U::asset_type);
		static_assert(is_detected_v<has_type_t, T>, "Asset type should have ::asset_type");*/

	public:
		// todo: try to deduce using TMP ?

		using load_from = typename T::load_from;

		//static constexpr AssetType asset_type = T::asset_type;
};

}

#endif // YAVE_ASSETS_ASSETTRAITS_H
