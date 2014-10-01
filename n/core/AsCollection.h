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

namespace internal {
	N_GEN_TYPE_HAS_METHOD(HasSize, size)
	N_GEN_TYPE_HAS_METHOD(HasIsSorted, isSorted)
	N_GEN_TYPE_HAS_METHOD(HasFindOne, findOne)
	N_GEN_TYPE_HAS_METHOD(HasCountAll, countAll)
	N_GEN_TYPE_HAS_METHOD(HasExistsOne, exitsOne)
	N_GEN_TYPE_HAS_METHOD(HasFind, find)
	N_GEN_TYPE_HAS_METHOD(HasExists, exists)
	N_GEN_TYPE_HAS_METHOD(HasCount, count)
	N_GEN_TYPE_HAS_METHOD(HasForeach, foreach)
	N_GEN_TYPE_HAS_METHOD(HasMapped, mapped)
	N_GEN_TYPE_HAS_METHOD(HasFiltered, filtered)
	N_GEN_TYPE_HAS_METHOD(HasForall, forall)
	N_GEN_TYPE_HAS_METHOD(HasMap, map)
	N_GEN_TYPE_HAS_METHOD(HasFilter, filter)

	N_GEN_TYPE_HAS_METHOD(HasShuffled, shuffled)
	N_GEN_TYPE_HAS_METHOD(HasReversed, reversed)
	N_GEN_TYPE_HAS_METHOD(HasShuffle, shuffle)
	N_GEN_TYPE_HAS_METHOD(HasReverse, reverse)
}

template<typename T>
class AsCollection
{
	private:
		template<typename U, bool CI, bool NCI> // false, false
		struct CollectionInternal
		{
			typedef NullType const_iterator;
			typedef NullType iterator;
			typedef NullType type;
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
			typedef NullType iterator;
			typedef typename TypeContent<const_iterator>::type type;
		};

		template<typename A, typename... B>
		struct first
		{
			typedef A type;
		};

		template<typename A, typename... B>
		struct last
		{
			typedef typename last<B...>::type type;
		};

		template<typename A>
		struct last<A>
		{
			typedef A type;
		};

		template<typename... A>
		struct VarArgs
		{
			typedef typename first<A...>::type first;
			typedef typename last<A...>::type last;
		};


		typedef CollectionInternal<T, TypeInfo<T>::isIterable, TypeInfo<T>::isNonConstIterable> InternalType;

		template<typename O, typename...>
		static O &makeOne();

		/*template<typename E, typename U>
		struct TemplateEngineInternal
		{

			typedef byte Yes[1];
			typedef byte No[2];
			template<typename V, template<typename...> class C>
			static Yes &test(C<V> &);

			template<typename... A>
			static No &test(...);



			static constexpr bool isTemplate = sizeof(test(makeOne<U>())) == sizeof(Yes);

			template<typename N, typename V, template<typename...> class C>
			static C<N> changeContentType(C<V> &);

			template<typename N>
			using ContainerType = typename std::result_of<decltype(changeContentType<N>(makeOne<U>()))>::type;
		};*/

		template<typename... Args>
		struct MappedType
		{
			typedef decltype(((T *)0)->template mapped<Args...>(makeOne<Args...>())) type;
		};


	public:
		typedef typename InternalType::const_iterator const_iterator;
		typedef typename InternalType::iterator iterator;
		typedef typename InternalType::type ElementType;

		AsCollection(const T &t) : collection(t) {
		}

		AsCollection(T &t) : collection(t) {
		}

		AsCollection(const AsCollection<T> &) = delete;

		template<typename... Args>
		void foreach(Args... f) const {
			foreachDispatch(BoolToType<internal::HasForeach<T, void, Args...>::value>(), f...);
		}

		template<typename... Args>
		bool isSorted(Args... f) const {
			return isSortedDispatch(BoolToType<internal::HasIsSorted<T, bool, Args...>::value>(), f...);
		}

		template<typename... Args>
		const_iterator findOne(Args... f) const {
			return findOneDispatch(BoolToType<internal::HasFindOne<T, const_iterator, Args...>::value>(), f...);
		}

		template<typename... Args>
		iterator findOne(Args... f) {
			return findOneDispatch(BoolToType<internal::HasFindOne<T, iterator, Args...>::value>(), f...);
		}

		template<typename... Args>
		uint countAll(Args... f) const {
			return countAllDispatch(BoolToType<internal::HasCountAll<T, uint, Args...>::value>(), f...);
		}

		template<typename... Args>
		bool existsOne(Args... f) const {
			return existsOneDispatch(BoolToType<internal::HasExistsOne<T, bool, Args...>::value>(), f...);
		}

		template<typename... Args>
		const_iterator find(Args... f) const {
			return findDispatch(BoolToType<internal::HasFind<T, const_iterator, Args...>::value>(), f...);
		}

		template<typename... Args>
		iterator find(Args... f) {
			return findDispatch(BoolToType<internal::HasFind<T, iterator, Args...>::value>(), f...);
		}

		template<typename... Args>
		uint count(Args... f) const {
			return countDispatch(BoolToType<internal::HasCount<T, uint, Args...>::value>(), f...);
		}

		template<typename... Args>
		bool exists(Args... f) const {
			return existsDispatch(BoolToType<internal::HasExists<T, bool, Args...>::value>(), f...);
		}

		template<typename... Args>
		bool forall(Args... f) const {
			return forallDispatch(BoolToType<internal::HasForall<T, bool, Args...>::value>(), f...);
		}

		template<typename... Args>
		void map(Args... f) {
			return mapDispatch(BoolToType<internal::HasMap<T, void, Args...>::value>(), f...);
		}

		template<typename... Args>
		void filter(Args... f) {
			return filterDispatch(BoolToType<internal::HasFilter<T, void, Args...>::value>(), f...);
		}

		template<typename... Args, typename C = T>
		C filtered(Args... f) const {
			return filteredDispatch<C, Args...>(BoolToType<internal::HasFiltered<T, C, Args...>::value>(), f...);
		}

		template<typename... Args, typename C = typename MappedType<Args...>::type>
		C mapped(Args... f) const {
			return mappedDispatch<C, Args...>(BoolToType<internal::HasMapped<T, C, Args...>::value>(), f...);
		}

		template<typename... Args>
		void shuffle(Args... f) {
			return shuffleDispatch(BoolToType<internal::HasShuffle<T, void, Args...>::value>(), f...);
		}

		template<typename... Args>
		void reverse(Args... f) {
			return reverseDispatch(BoolToType<internal::HasReverse<T, void, Args...>::value>(), f...);
		}

		template<typename... Args, typename C = T>
		C shuffled(Args... f) {
			return shuffledDispatch<C, Args...>(BoolToType<internal::HasShuffled<T, C, Args...>::value>(), f...);
		}

		template<typename... Args, typename C = T>
		C reversed(Args... f) {
			return reversedDispatch<C, Args...>(BoolToType<internal::HasReversed<T, C, Args...>::value>(), f...);
		}

	private:
		T collection;

		template<typename U>
		void foreachDispatch(FalseType, const U &f) const {
			std::for_each(collection.begin(), collection.end(), f);
		}

		template<typename... Args>
		bool isSortedDispatch(TrueType, Args... f) const {
			return collection.isSorted(f...);
		}

		template<typename... Args>
		const_iterator findOneDispatch(TrueType, Args... f) const {
			return collection.findOne(f...);
		}

		template<typename... Args>
		iterator findOneDispatch(TrueType, Args... f) {
			return collection.findOne(f...);
		}

		template<typename... Args>
		uint countAllDispatch(TrueType, Args... f) const {
			return collection.countAll(f...);
		}

		template<typename... Args>
		bool existsOneDispatch(TrueType, Args... f) const {
			return collection.existsOne(f...);
		}

		template<typename... Args>
		const_iterator findDispatch(TrueType, Args... f) const {
			return collection.find(f...);
		}

		template<typename... Args>
		iterator findDispatch(TrueType, Args... f) {
			return collection.find(f...);
		}

		template<typename... Args>
		uint countDispatch(TrueType, Args... f) const {
			return collection.count(f...);
		}

		template<typename... Args>
		bool existsDispatch(TrueType, Args... f) const {
			return collection.exists(f...);
		}

		template<typename... Args>
		bool forallDispatch(TrueType, Args... f) const {
			return collection.forAll(f...);
		}

		template<typename... Args>
		void mapDispatch(TrueType, Args... f) {
			collection.map(f...);
		}

		template<typename... Args>
		void filterDispatch(TrueType, Args... f) {
			collection.filter(f...);
		}

		template<typename C, typename... Args>
		C filteredDispatch(TrueType, Args... f) const {
			return collection.template filtered<Args...>(f...);
		}

		template<typename C, typename... Args>
		C mappedDispatch(TrueType, Args... f) const {
			return collection.template mapped<Args...>(f...);
		}

		template<typename... Args>
		void reverseDispatch(TrueType, Args... f) {
			collection.reverse(f...);
		}

		template<typename... Args>
		void shuffleDispatch(TrueType, Args... f) {
			collection.shuffle(f...);
		}

		template<typename C, typename... Args>
		C reversedDispatch(TrueType, Args... f) const {
			return collection.template reversed<Args..., C>(f...);
		}

		template<typename C, typename... Args>
		C shuffledDispatch(TrueType, Args... f) const {
			return collection.template shuffled<Args..., C>(f...);
		}


};

}
}

#endif // COLLECTION_H
