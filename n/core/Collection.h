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
		template<typename U, bool CI, bool NCI> // false, false
		struct CollectionInternal
		{
			typedef NullType const_iterator;
			typedef NullType iterator;
			typedef NullType type;
			typedef NullType SubCollection;
		};

		template<typename U>
		struct CollectionInternal<U, true, true>
		{
			using const_iterator = typename U::const_iterator;
			using iterator = typename U::iterator;
			typedef typename TypeContent<iterator>::type type;
		};

		template<typename U>
		struct CollectionInternal<U, true, false>
		{
			using const_iterator = typename U::const_iterator;
			typedef const_iterator iterator;
			typedef typename TypeContent<const_iterator>::type type;
		};

		typedef CollectionInternal<T, TypeInfo<T>::isIterable, TypeInfo<T>::isNonConstIterable> InternalType;

		Collection() = delete;

	public:
		typedef typename InternalType::const_iterator const_iterator;
		typedef typename InternalType::iterator iterator;
		typedef typename InternalType::type Element;

		static constexpr bool isCollection = !std::is_same<Element, NullType>::value;

		template<typename U>
		struct isCollectionOf
		{
			static constexpr bool value = isCollection && TypeConversion<Element, U>::exists;
		};
};

template<typename C, typename ElementType>
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
		static constexpr bool isCollectionOfCompatibles = Compat<C, !std::is_same<C, ElementType>::value && Collection<C>::isCollection>::value;
		static constexpr bool value = !std::is_same<C, ElementType>::value &&
										 !(TypeConversion<C, const ElementType>::exists ||
										   (TypeConversion<C, const ElementType>::canBuild &&
											!isCollectionOfCompatibles));

		typedef BoolToType<value> type;

};

}
}

#endif // COLLECTION_H
