/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#ifndef YAVE_ASSETS_ASSETID_H
#define YAVE_ASSETS_ASSETID_H

#include "AssetTraits.h"

namespace yave {

struct AssetId {
    public:
        constexpr AssetId() {
        }

        static constexpr AssetId invalid_id() {
            return AssetId();
        }

        static constexpr AssetId from_id(u64 id) {
            return AssetId(id);
        }

        /*constexpr AssetType type() const {
            return AssetType(_id & _type_mask);
        }*/

        /*constexpr bool is_valid() const {
            return _id != _invalid_id;
        }*/

        constexpr explicit operator u64() const {
            return _id;
        }

        constexpr u64 id() const {
            return _id;
        }

        constexpr std::strong_ordering operator<=>(const AssetId& other) const {
            return _id <=> other._id;
        }

        constexpr bool operator==(const AssetId&) const = default;
        constexpr bool operator!=(const AssetId&) const = default;

    private:
        constexpr AssetId(u64 id) : _id(id) {
        }

    private:
        static constexpr u64 _invalid_id = 0xFFFFFFFFFFFFFFFF;

        u64 _id = _invalid_id;
};

core::String stringify_id(AssetId id);

static_assert(sizeof(AssetId) == sizeof(i64));
static_assert(AssetId() == AssetId::invalid_id());
static_assert(std::is_trivially_copyable_v<AssetId>);

}



template<>
struct std::hash<yave::AssetId> : y::Hash<y::u64> {
    auto operator()(const yave::AssetId& id) const {
        return y::Hash<y::u64>::operator()(id.id());
    }
};

#endif // YAVE_ASSETS_ASSETID_H

