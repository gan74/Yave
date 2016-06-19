/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_CORE_COLLECTION_H
#define N_CORE_COLLECTION_H

#include <n/types.h>
#include <algorithm>

namespace n {
namespace core {

template<typename T>
class Collection
{
	private:
		struct NotACollection
		{
			using const_iterator = details::NullType;
			using iterator = details::NullType;
			using type = details::NullType;
		};

		template<typename U>
		struct NonConstCollection
		{
			using const_iterator = typename U::const_iterator;
			using iterator = typename U::iterator;
			using type = typename TypeContent<iterator>::type;
		};

		template<typename U>
		struct ConstCollection
		{
			using const_iterator = typename U::const_iterator;
			using iterator = const_iterator;
			using type = typename TypeContent<const_iterator>::type;
		};

		using InternalType = typename If<TypeInfo<T>::isNonConstIterable,
										  NonConstCollection<T>,
										  typename If<TypeInfo<T>::isIterable, ConstCollection<T>, NotACollection>::type
										>::type;

		Collection() = delete;

	public:
		using const_iterator = typename InternalType::const_iterator;
		using iterator = typename InternalType::iterator;
		using Element = typename InternalType::type;

		static constexpr bool isCollection = !std::is_same<Element, details::NullType>::value;

		template<typename U>
		struct isCollectionOf
		{
			static constexpr bool value = isCollection && TypeConversion<Element, U>::exists;
		};
};

template<typename PotentialCollection, typename ElementType>
struct ShouldInsertAsCollection
{
	template<typename E, bool Col>
	struct Compat // E, true
	{
		static constexpr bool value = TypeConversion<typename Collection<E>::Element, const ElementType>::existsWeak || Compat<typename Collection<E>::Element, Collection<E>::isCollection>::value;
	};

	template<typename E>
	struct Compat<E, false>
	{
		static constexpr bool value = false;
	};

	public:
		static constexpr bool isCollectionOfCompatibles = Compat<PotentialCollection, !std::is_same<PotentialCollection, ElementType>::value && Collection<PotentialCollection>::isCollection>::value;
		static constexpr bool isElementType = std::is_same<PotentialCollection, ElementType>::value;
		static constexpr bool canConvertToElement = TypeConversion<PotentialCollection, const ElementType>::exists;
		static constexpr bool canBuildElementFrom = TypeConversion<PotentialCollection, const ElementType>::canBuild && !isCollectionOfCompatibles;


		static constexpr bool value = !(isElementType || canConvertToElement || (canBuildElementFrom && !isCollectionOfCompatibles));

		using type = BoolToType<value>;

};

template<typename T>
struct Inserter
{
	using Element = typename Collection<T>::Element;

	T &operator()(const Element &e) {
		t.insert(e);
		return t;
	}

	Inserter(T &w) : t(w) {
	}

	private:
		T &t;
};

template<typename T>
Inserter<T> inserter(T &t) {
	return Inserter<T>(t);
}

}
}

#endif // COLLECTION_H
