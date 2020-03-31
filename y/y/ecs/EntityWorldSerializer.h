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
#ifndef Y_ECS_ENTITYWORLDSERIALIZER_H
#define Y_ECS_ENTITYWORLDSERIALIZER_H

#include "EntityWorld.h"

#include <y/serde3/archives.h>
#include <y/serde3/poly.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <utility>

namespace y {
namespace ecs {
namespace detail {
void set_serializer_world(EntityWorld* world);
void reset_serializer_world();
EntityWorld* serializer_world();
}


class EntityIDSerializer {
	public:
		using value_type = u32;

		~EntityIDSerializer();

		void insert(u32 index);


		auto begin() const {
			auto extract_index = [](EntityID id) { return id.index(); };
			return TransformIterator(detail::serializer_world()->entity_ids().begin(), extract_index);
		}

		auto end() const {
			return EndIterator();
		}
};

class ComponentSerializerBase : NonMovable {
	public:
		virtual ~ComponentSerializerBase();

		y_serde3_poly_base(ComponentSerializerBase)

	protected:
		template<typename T>
		struct extract_component {
			std::pair<EntityID, T*> operator()(const EntityData& data) const {
				y_debug_assert(data.id.is_valid());
				return {data.id, detail::serializer_world()->find_component<T>(data.id)};
			}
		};

		template<typename T>
		struct non_null {
			bool operator()(const std::pair<EntityID, T*>& p) const {
				return p.second;
			}
		};

		template<typename T>
		struct deref {
			std::pair<u32, T&> operator()(const std::pair<EntityID, T*>& p) const {
				return {p.first.index(), *p.second};
			}
		};


		template<typename T>
		static auto index_components() {
			const auto& entities = detail::serializer_world()->_entities;
			auto components = TransformIterator(entities.begin(), extract_component<T>());
			auto non_nulls = FilterIterator(components, entities.end(), non_null<T>());
			return core::Range(TransformIterator(non_nulls, deref<T>()), EndIterator());
		}
};

template<typename T>
class ComponentSerializer final : public ComponentSerializerBase {
	class SerializerView {
		using view_type = decltype(index_components<T>());
		static_assert(std::is_copy_constructible_v<view_type>);

		public:
			using value_type = typename view_type::value_type;

			~SerializerView() {
			}

			void insert(std::pair<u32, T> component) {
				Y_TODO(This might be really slow)
				const EntityID id(component.first);
				detail::serializer_world()->add_component<T>(id);
				*detail::serializer_world()->component<T>(id) = std::move(component.second);
			}

			auto begin() const {
				return _view.begin();
			}

			auto end() const {
				return _view.end();
			}

			// we need this for post_deserialization
			auto begin() {
				init_view();
				return _view.begin();
			}

			auto end() {
				init_view();
				return _view.end();
			}

		private:
			void init_view() {
				if(_view.is_empty()) {
					_view = index_components<T>();
				}
			}

			view_type _view = index_components<T>();
	};

	public:
		ComponentSerializer() = default;

		y_serde3_poly(ComponentSerializer)
		y_serde3(components())

	private:
		SerializerView components() const {
			return SerializerView();
		}
};

class EntityWorldSerializer : NonMovable {
	public:
		EntityWorldSerializer(EntityWorld* world) {
			detail::set_serializer_world(world);
		}

		~EntityWorldSerializer() {
			detail::reset_serializer_world();
		}

		y_serde3(_ids, _components)

	private:
		EntityIDSerializer _ids;
		ComponentSerializer<int> _components;
};


}
}

#endif // Y_ECS_ENTITYWORLDSERIALIZER_H
