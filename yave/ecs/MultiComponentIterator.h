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
#ifndef YAVE_ECS_MULTICOMPONENTITERATOR_H
#define YAVE_ECS_MULTICOMPONENTITERATOR_H

#include "ecs.h"
#include "ComponentBitmask.h"
#include "ComponentContainer.h"

namespace yave {
namespace ecs {

struct ReturnIdPolicy {
	using value_type = EntityId;
	using reference = const EntityId&;

	template<typename It, typename E>
	const EntityId& operator()(It it, const E&) const {
		return *it;
	}
};

struct ReturnEntityPolicy {
	using value_type = Entity;
	using reference = Entity&;

	template<typename It, typename E>
	auto&& operator()(It it, E&& entities) const {
		return entities[*it];
	}
};


namespace detail {
template<typename T, typename... Args>
std::tuple<T&, Args&...> map_component_ids(const Entity& entity, ComponentContainer<T>* head, ComponentContainer<Args>*... tail) {
	if constexpr(sizeof...(Args)) {
		return std::tuple_cat(map_component_ids(entity, head), map_component_ids(entity, tail...));
	} else {
		T* component = head->component(entity);
		y_debug_assert(component);
		return std::tie(*component);
	}
}
}

template<bool Const, typename... Args>
class ReturnComponentsPolicy {
	public:
		using value_type = std::conditional_t<Const, std::tuple<const Args&...>, std::tuple<Args&...>>;
		using reference = value_type;

		ReturnComponentsPolicy(const std::tuple<ComponentContainer<Args>*...>& containers) : _containers(containers) {
		}

		template<typename It, typename E>
		value_type operator()(It it, E&& entities) const {
			return std::apply(detail::map_component_ids<Args...>, std::tuple_cat(std::make_tuple(entities[*it]), _containers));
		}

	private:
		std::tuple<ComponentContainer<Args>*...> _containers;
};




class MultiComponentIteratorEndSentry {};

template<typename It, typename ReturnPolicy = ReturnEntityPolicy, bool Const = false>
class MultiComponentIterator : ReturnPolicy {

	template<typename T>
	using add_const_to_ref_t = std::conditional_t<std::is_reference_v<T>, const std::remove_reference_t<T>&, T>;

	public:
		static constexpr bool is_const = Const;
		using end_iterator = MultiComponentIteratorEndSentry;

		using value_type = std::conditional_t<is_const, std::add_const_t<typename ReturnPolicy::value_type>, typename ReturnPolicy::value_type>;
		using reference = std::conditional_t<is_const, add_const_to_ref_t<typename ReturnPolicy::reference>, typename ReturnPolicy::reference>;
		using pointer = std::add_pointer_t<value_type>;
		using difference_type = usize;

		// https://stackoverflow.com/questions/46626832/why-are-c-iterators-required-to-return-a-reference
		using iterator_category = std::conditional_t<std::is_reference_v<reference>, std::forward_iterator_tag, std::input_iterator_tag>;


		bool at_end() const {
			return _iterator == _end;
		}

		const MultiComponentIterator& operator++() {
			do {
				++_iterator;
			} while(!at_end() && !has_all_bits());
			return *this;
		}

		MultiComponentIterator operator++(int) {
			MultiComponentIterator it = *this;
			operator++();
			return it;
		}

		auto&& entity() const {
			return _entities[*_iterator];
		}

		reference get() const {
			y_debug_assert(_iterator->is_valid());
			return ReturnPolicy::operator()(_iterator, _entities);
		}

		pointer operator->() const {
			static_assert(std::is_reference_v<decltype(get())>);
			return &get();
		}

		reference operator*() const {
			return get();
		}


		bool operator==(const MultiComponentIterator& other) const {
			return _iterator == other._iterator;
		}

		bool operator!=(const MultiComponentIterator& other) const {
			return _iterator != other._iterator;
		}

		bool operator==(const end_iterator&) const {
			return at_end();
		}

		bool operator!=(const end_iterator&) const {
			return !at_end();
		}

	private:
		friend class EntityWorld;

		using entity_container = std::conditional_t<is_const, const SlotMap<Entity, EntityTag>, SlotMap<Entity, EntityTag>>;
		using entity_container_ref = std::add_lvalue_reference_t<entity_container>;

		MultiComponentIterator(It it, It end, entity_container_ref entities, const ComponentBitmask& bits, const ReturnPolicy& policy = ReturnPolicy()) :
				ReturnPolicy(policy),
				_iterator(it),
				_end(end),
				_component_type_bits(bits),
				_entities(entities) {

			while(!at_end() && !has_all_bits()) {
				++_iterator;
			}
		}

		bool has_all_bits() const {
			return _iterator->is_valid() && (entity().components_bits() & _component_type_bits) == _component_type_bits;
		}


		It _iterator;
		It _end;
		ComponentBitmask _component_type_bits;
		entity_container_ref _entities;
};

namespace {
template<typename T>
static constexpr bool is_const_ref_v = std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>;
static_assert(is_const_ref_v<const int&> && !is_const_ref_v<int&> && !is_const_ref_v<int> && !is_const_ref_v<const int>);
static_assert(!std::is_same_v<std::tuple<int&, float&>, std::tuple<const int&, const float&>>);

template<typename Policy, bool Const = false>
using MCI = MultiComponentIterator<yave::ecs::EntityId*, Policy, Const>;

static_assert(!MCI<ReturnEntityPolicy>::is_const);
static_assert(MCI<ReturnEntityPolicy, true>::is_const);

static_assert(std::is_reference_v<typename MCI<ReturnEntityPolicy>::reference>);
static_assert(std::is_reference_v<typename MCI<ReturnEntityPolicy, true>::reference>);

static_assert(!is_const_ref_v<typename MCI<ReturnEntityPolicy>::reference>);
static_assert(is_const_ref_v<typename MCI<ReturnEntityPolicy, true>::reference>);

static_assert(std::is_reference_v<decltype(*std::declval<MCI<ReturnEntityPolicy>>())>);
static_assert(std::is_reference_v<decltype(*std::declval<MCI<ReturnEntityPolicy, true>>())>);

static_assert(is_const_ref_v<typename MCI<ReturnEntityPolicy, true>::reference>);
static_assert(is_const_ref_v<decltype(*std::declval<MCI<ReturnEntityPolicy, true>>())>);

static_assert(!std::is_reference_v<typename MCI<ReturnComponentsPolicy<false, int, float>>::reference>);
static_assert(!std::is_reference_v<typename MCI<ReturnComponentsPolicy<true, int, float>, true>::reference>);

static_assert(std::is_same_v<typename MCI<ReturnComponentsPolicy<false, int, float>>::reference, std::tuple<int&, float&>>);
static_assert(std::is_same_v<typename MCI<ReturnComponentsPolicy<true, int, float>>::reference, std::tuple<const int&, const float&>>);

static_assert(!std::is_reference_v<decltype(*std::declval<MCI<ReturnComponentsPolicy<false, int, float>>>())>);
static_assert(!std::is_reference_v<decltype(*std::declval<MCI<ReturnComponentsPolicy<true, int, float>>>())>);
}

}
}

namespace std {
template<typename It, typename ReturnPolicy, bool Const>
struct iterator_traits<yave::ecs::MultiComponentIterator<It, ReturnPolicy, Const>> {
	using iterator_type = yave::ecs::MultiComponentIterator<It, ReturnPolicy, Const>;
	using value_type = typename iterator_type::value_type;
	using reference = typename iterator_type::reference;
	using pointer = typename iterator_type::pointer;
	using difference_type = typename iterator_type::difference_type;
	using iterator_category = typename iterator_type::iterator_category;
};

template<>
struct iterator_traits<yave::ecs::MultiComponentIteratorEndSentry> :
			iterator_traits<yave::ecs::MultiComponentIterator<yave::ecs::EntityId*>> {
};

}

#endif // YAVE_ECS_MULTICOMPONENTITERATOR_H
