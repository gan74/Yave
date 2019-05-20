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
#ifndef YAVE_ECS_ENTITYID_H
#define YAVE_ECS_ENTITYID_H

#include <yave/yave.h>

namespace yave {
namespace ecs {

class EntityId {
	public:
		using index_type = u32;

		constexpr EntityId() = default;

		static EntityId from_unversioned_index(index_type index);

		index_type index() const;

		bool is_valid() const;

		bool operator==(const EntityId& other) const;
		bool operator!=(const EntityId& other) const;

	private:
		friend class EntityIdPool;

		void clear();
		void set(index_type index);

		static constexpr index_type invalid_index = index_type(-1);
		index_type _index = invalid_index;
		index_type _version = invalid_index;
};

using EntityIndex = typename EntityId::index_type;

static_assert(sizeof(EntityId) == sizeof(u64));
static_assert(std::is_trivially_copyable_v<EntityId>);

/*class EntityIndex : public EntityId {
	public:
		constexpr EntityIndex() = default;

		constexpr EntityIndex(const EntityId& id) : EntityId(id) {
		}

		operator usize() const {
			return index();
		}
};


struct EntityIndexTraits {
	static constexpr EntityIndex invalid_index = EntityIndex();
};

static_assert(sizeof(EntityIndex) == sizeof(u64));
static_assert(std::is_trivially_copyable_v<EntityIndex>);
static_assert(std::is_convertible_v<EntityIndex, EntityId>);
static_assert(std::is_convertible_v<EntityId, EntityIndex>);*/

}
}

#endif // YAVE_ECS_ENTITYID_H
