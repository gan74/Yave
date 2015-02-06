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
#include "Option.h"

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
	N_GEN_TYPE_HAS_METHOD(HasMake, make)
	N_GEN_TYPE_HAS_METHOD(HasSetMinCapacity, setMinCapacity)

	N_GEN_TYPE_HAS_METHOD(HasEquals, operator==)
}


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

			class SubCollection
			{
				public:
					typedef CollectionInternal::iterator iterator;
					typedef CollectionInternal::const_iterator const_iterator;

					iterator begin() {
						return beg;
					}

					iterator end() {
						return end;
					}

					const_iterator begin() const {
						return beg;
					}

					const_iterator end() const {
						return en;
					}

				private:
					SubCollection(iterator b, iterator e) :	beg(b), en(e) {
					}

					iterator beg;
					iterator en;
			};
		};

		template<typename U>
		struct CollectionInternal<U, true, false>
		{
			using const_iterator = typename U::const_iterator;
			typedef NullType iterator;
			typedef typename TypeContent<const_iterator>::type type;

			class SubCollection
			{
				public:
					typedef CollectionInternal::const_iterator const_iterator;

					const_iterator begin() const {
						return beg;
					}

					const_iterator end() const {
						return en;
					}

				private:
					friend class Collection<T>;

					SubCollection(const_iterator b, const_iterator e) :	beg(b), en(e) {
					}

					const_iterator beg;
					const_iterator en;
			};
		};

		typedef CollectionInternal<T, TypeInfo<T>::isIterable, TypeInfo<T>::isNonConstIterable> InternalType;

		template<typename O, typename...>
		static O &makeOne();

		template<typename... Args>
		struct MappedType
		{
			typedef decltype(((T *)0)->template mapped<Args...>(makeOne<Args...>())) type;
		};

	private:
		template<typename U>
		friend constexpr Collection<U> AsCollection(U &);

		template<typename U>
		friend constexpr Collection<const U> AsCollection(const U &);

		Collection(T &t) : collection(t) {
		}

		Collection(const Collection<T> &c) : collection(c.collection){

		}

	public:
		typedef typename InternalType::const_iterator const_iterator;
		typedef typename InternalType::iterator iterator;
		typedef typename InternalType::type ElementType;
		typedef typename InternalType::SubCollection SubCollection;

		static constexpr bool isCollection = !std::is_same<ElementType, NullType>::value;

		template<typename U>
		struct isCollectionOf
		{
			static constexpr bool value = isCollection && TypeConversion<ElementType, U>::exists;
		};

		iterator begin() {
			return collection.begin();
		}

		iterator end() {
			return collection.end();
		}

		const_iterator begin() const {
			return collection.begin();
		}

		const_iterator end() const {
			return collection.end();
		}

		bool isEmpty() const {
			return !(this->size());
		}

		Option<uint> sizeOption() const {
			if(internal::HasSize<T, uint>::value) {
				return size();
			}
			return None();
		}

		uint size() const {
			return sizeDispatch(BoolToType<internal::HasSize<T, uint>::value>());
		}

		template<typename U, typename... Args>
		U make(U u, Args... f) const {
			return makeDispatch(BoolToType<internal::HasMake<T, U, U, Args...>::value>(), u, f...);
		}

		template<typename... Args>
		void foreach(Args... f) const {
			foreachDispatch(BoolToType<internal::HasForeach<T, void, Args...>::value>(), f...);
		}

		template<typename... Args>
		void foreach(Args... f) {
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
		void setMinCapacity(Args... f) {
			return setMinCapacityDispatch(BoolToType<internal::HasSetMinCapacity<T, void, Args...>::value>(), f...);
		}

		Collection<T> sub(iterator be, iterator en) {
			return Collection<T>(SubCollection(be, en));
		}

		Collection<const T> sub(const_iterator be, const_iterator en) const {
			return Collection<const T>(SubCollection(be, en));
		}

		template<typename... Args>
		bool operator==(Args... f) const {
			return equalsDispatch(BoolToType<internal::HasEquals<T, bool, Args...>::value>(), f...);
		}

		template<typename... Args>
		bool operator!=(Args... f) const {
			return !operator==(f...);
		}

	private:
		T &collection;

		template<typename... Args>
		bool equalsDispatch(TrueType, Args... f) const {
			return collection.operator==(f...);
		}

		template<typename C>
		bool equalsDispatch(FalseType, const C &c) const {
			if(internal::HasSize<C, uint>::value && internal::HasSize<T, uint>::value && Collection(c).size() != size()) {
				return false;
			}
			const_iterator i = collection.begin();
			for(typename C::const_iterator it = c.begin(), en = c.end(); it != en; ++it) {
				if(!(*it == *i)) {
					return false;
				}
				++i;
			}
			return true;
		}

		template<typename... Args>
		void setMinCapacityDispatch(TrueType, Args... f) {
			collection.setMinCapacity(f...);
		}

		template<typename... Args>
		void setMinCapacityDispatch(FalseType, Args...) {
		}


		uint sizeDispatch(TrueType) const {
			return collection.size();
		}

		template<typename U, typename... Args>
		U makeDispatch(TrueType, const U &u, Args... f) const {
			return collection.template make<U, Args...>(u, f...);
		}

		template<typename... Args>
		void foreachDispatch(TrueType, Args... f) const {
			collection.foreach(f...);
		}

		template<typename... Args>
		void foreachDispatch(TrueType, Args... f) {
			collection.foreach(f...);
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

		//--------------- defaults ---------------


		uint sizeDispatch(FalseType) const {
			uint s = 0;
			foreach([&s](const ElementType &) { s++; });
			return s;
		}

		template<typename U>
		void foreachDispatch(FalseType, const U &f) const {
			std::for_each(collection.begin(), collection.end(), f);
		}

		template<typename U>
		void foreachDispatch(FalseType, const U &f) {
			std::for_each(collection.begin(), collection.end(), f);
		}

		bool isSortedDispatch(FalseType) const {
			const_iterator l = collection.begin();
			for(const_iterator it = ++collection.begin(); it != collection.end(); ++it) {
				if(*it < *l) {
					return false;
				}
				l = it;
			}
			return true;
		}

		template<typename U>
		iterator findOneDispatch(FalseType, const U &f, const_iterator from) {
			for(iterator i = const_cast<iterator>(from); i != collection.end(); ++i) {
				if(f(*i)) {
					return i;
				}
			}
			return collection.end();
		}

		template<typename U>
		const_iterator findOneDispatch(FalseType, const U &f, const_iterator from) const {
			for(const_iterator i = from; i != collection.end(); ++i) {
				if(f(*i)) {
					return i;
				}
			}
			return collection.end();
		}

		template<typename U>
		uint countAllDispatch(FalseType, const U &f) const {
			uint c = 0;
			for(const_iterator i = collection.begin(); i != collection.end(); ++i) {
				if(f(*i)) {
					c++;
				}
			}
			return c;
		}

		template<typename V>
		bool existsOneDispatch(FalseType, const V &f) const {
			for(const_iterator i = collection.begin(); i != collection.end(); ++i) {
				if(f(*i)) {
					return true;
				}
			}
			return false;
		}

		template<typename U>
		iterator findDispatch(FalseType, const U &f, const_iterator from) {
			return findOne(f, from);
		}

		template<typename U>
		const_iterator findDispatch(FalseType, const U &f, const_iterator from) const {
			return findOne(f, from);
		}

		template<typename U>
		uint countDispatch(FalseType, const U &f) const {
			return countAll(f);
		}

		template<typename V>
		bool existsDispatch(FalseType, const V &f) const {
			return existsOne(f);
		}

		iterator findDispatch(FalseType, const ElementType &e) {
			return findOne([&](const ElementType &t) { return t == e; }, collection.begin());
		}

		iterator findDispatch(FalseType, const ElementType &e, const_iterator from) {
			return findOne([&](const ElementType &t) { return t == e; }, from);
		}

		const_iterator findDispatch(FalseType, const ElementType &e) const {
			return findOne([&](const ElementType &t) { return t == e; }, collection.begin());
		}

		const_iterator findDispatch(FalseType, const ElementType &e, const_iterator from) const {
			return findOne([&](const ElementType &t) { return t == e; }, from);
		}

		uint countDispatch(FalseType, const ElementType &e) const {
			return countAll([&](const ElementType &t) { return t == e; });
		}

		bool existsDispatch(FalseType, const ElementType &e) const {
			return existsOne([&](const ElementType &t) { return t == e; });
		}

		template<typename C, typename V>
		C mappedDispatch(FalseType, const V &f) const {
			C a;
			Collection(a).setMinCapacity(size());
			foreach([&](const ElementType &e) { a.insert(f(e)); });
			return a;
		}

		template<typename C, typename U>
		C filteredDispatch(FalseType, const U &f) const {
			C a;
			Collection(a).setMinCapacity(size());
			foreach([&](const ElementType &e) {
				if(f(e)) {
					a.insert(e);
				}
			});
			return a;
		}

		template<typename U>
		bool forallDispatch(FalseType, const U &f) const {
			for(const ElementType &t : *this) {
				if(!f(t)) {
					return false;
				}
			}
			return true;
		}

		template<typename V>
		void mapDispatch(FalseType, const V &f) {
			std::for_each(collection.begin(), collection.end(), [&](T &e) { e = f(e); });
		}

		template<typename U>
		void filterDispatch(FalseType, const U &f) {
			for(iterator it = collection.begin(); it != collection.end();) {
				if(!f(*it)) {
					it = collection.remove(it);
				} else {
					++it;
				}
			}
		}

		template<typename U>
		U makeDispatch(FalseType, const U &f) const {
			U str;
			if(this->isEmpty()) {
				return U();
			}
			const_iterator end = collection.end();
			--end;
			for(const_iterator it = collection.begin(); it != end; ++it) {
				str += U(*it) + f;
			}
			return str + U(*end);
		}
};

template<typename T>
constexpr Collection<T> AsCollection(T &t) {
	return Collection<T>(t);
}

template<typename T>
constexpr Collection<const T> AsCollection(const T &t) {
	return Collection<const T>(t);
}


}
}

#endif // COLLECTION_H
