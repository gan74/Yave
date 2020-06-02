/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_ECS_ECS_H
#define Y_ECS_ECS_H

#include <y/utils.h>

namespace y {
namespace ecs {

class EntityWorld;
class Archetype;

class ComponentInfoSerializerBase;
class ComponentSerializerWrapper;
class ComponentContainerBase;


static constexpr usize entities_per_chunk = 1024;

namespace detail {
u32 next_type_index();
}

template<typename T>
u32 type_index() {
	static u32 index = detail::next_type_index();
	return index;
}



class EntityID {
	public:
		explicit EntityID(u32 index = invalid_index, u32 version = 0) : _index(index), _version(version) {
		}

		void invalidate() {
			_index = invalid_index;
		}

		u32 index() const {
			return _index;
		}

		u32 version() const {
			return _version;
		}

		bool is_valid() const {
			return _index != invalid_index;
		}

		u64 as_u64() const {
			return (u64(_index) << 32) | _version;
		}

		bool operator==(const EntityID& other) const {
			return _index == other._index && _version == other._version;
		}

		bool operator!=(const EntityID& other) const {
			return !operator==(other);
		}

	private:
		static constexpr u32 invalid_index = u32(-1);

		u32 _index = invalid_index;
		u32 _version = 0;
};



struct EntityData {
	EntityID id;
	Archetype* archetype = nullptr;
	usize archetype_index = usize(-1);

	void invalidate() {
		// Keep the version
		id.invalidate();
		archetype = nullptr;
	}

	bool is_valid() const {
		return id.is_valid();
	}
};



template<typename... Args>
struct StaticArchetype {
	static constexpr usize component_count = sizeof...(Args);

	template<typename... E>
	using with = StaticArchetype<Args..., E...>;
};



}
}

#endif // Y_ECS_ECS_H
