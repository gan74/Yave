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
#ifndef YAVE_ASSETS_ASSET_H
#define YAVE_ASSETS_ASSET_H

#include "AssetTraits.h"
#include <yave/utils/serde.h>

namespace yave {

struct AssetId {
	public:
		constexpr AssetId() {
		}

		static constexpr AssetId invalid_id() {
			return AssetId();
		}

		/*constexpr AssetType type() const {
			return AssetType(_id & _type_mask);
		}*/

		/*constexpr bool is_valid() const {
			return _id != _invalid_id;
		}*/

		constexpr operator u64() const {
			return _id;
		}

		constexpr u64 id() const {
			return _id;
		}

		constexpr bool operator==(AssetId other) const {
			return _id == other._id;
		}

		constexpr bool operator!=(AssetId other) const {
			return _id != other._id;
		}

		y_serde2(_id)

	private:
		friend class AssetIdFactory;

		/*constexpr AssetId(i64 id, AssetType type) : _id((id << _id_offset) | i64(type)) {
		}*/

		constexpr AssetId(u64 id) : _id(id) {
		}

	private:
		static constexpr u64 _invalid_id = 0xFFFFFFFFFFFFFFFF;

		u64 _id = _invalid_id;
};

static_assert(sizeof(AssetId) == sizeof(i64));
static_assert(AssetId() == AssetId::invalid_id());
//static_assert(AssetId().type() == AssetType::Unknown);
static_assert(std::is_trivially_copyable_v<AssetId>);

class AssetIdFactory {
	public:
		AssetId create_id() {
			return AssetId(_next_id++);
		}

	private:
		u64 _next_id = 0;
};

}



namespace std {
template<>
struct hash<yave::AssetId> : hash<yave::u64> {
	auto operator()(const yave::AssetId& id) const {
		return hash<yave::u64>::operator()(id);
	}
};
}

#endif // YAVE_ASSETS_ASSET_H
