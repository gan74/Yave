/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#include <n/Types.h>

namespace n {
namespace core {

template<typename T>
class AsCollection
{
	private:
		template<typename U, bool CI, bool NCI> // false, false
		struct CollectionInternal
		{
			typedef NullType ConstIteratorType;
			typedef NullType IteratorType;
			typedef NullType type;
		};

		template<typename U>
		struct CollectionInternal<U, true, true>
		{
			using ConstIteratorType = typename U::const_iterator;
			using IteratorType = typename U::iterator;
			typedef typename TypeContent<IteratorType>::type type;
		};

		template<typename U>
		struct CollectionInternal<U, true, false>
		{
			using ConstIteratorType = typename U::const_iterator;
			typedef NullType IteratorType;
			typedef typename TypeContent<ConstIteratorType>::type type;
		};


		typedef CollectionInternal<T, TypeInfo<T>::isIterable, TypeInfo<T>::isNonConstIterable> InternalType;
		T &makeT();

		template<typename E, typename U>
		struct TemplateEngineInternal
		{

			typedef byte Yes[1];
			typedef byte No[2];
			template<typename V, template<typename...> class C>
			static Yes &test(C<V> &);

			template<typename... A>
			static No &test(...);

			template<typename O>
			static O &makeOne();

			static constexpr bool isTemplate = sizeof(test(makeOne<U>())) == sizeof(Yes);

			template<typename N, typename V, template<typename...> class C>
			static C<N> changeContentType(C<V> &);

			template<typename N>
			using ContainerType = typename std::result_of<decltype(changeContentType<N>(makeOne<U>()))>::type;
		};


	public:
		typedef typename InternalType::ConstIteratorType ConstIteratorType;
		typedef typename InternalType::IteratorType IteratorType;
		typedef typename InternalType::type ElementType;

		AsCollection(const T &t) : collection(t) {
		}

		AsCollection(T &t) : collection(t) {
		}

		AsCollection(const AsCollection<T> &) = delete;

	private:
		T collection;

};

}
}

#endif // COLLECTION_H
