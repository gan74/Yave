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

#ifndef N_CORE_ARRAY_H
#define N_CORE_ARRAY_H

#include <n/types.h>
#include <n/utils.h>
#include "Collection.h"

namespace n {
namespace core {

class DefaultArrayResizePolicy
{
	public:
		static uint standard(uint size) {
			static const uint offset = 12;
			uint l = log2ui(size + offset);
			return size < 2 ? 8 : (1 << (l + 1)) - offset;
		}

	protected:
		uint size(uint size) const {
			return standard(size);
		}

		bool shrink(uint, uint) const {
			return false;
		}
};

class CompactArrayResizePolicy
{
	protected:
		uint size(uint size) const {
			static const uint linearThreshold = 11; // 2048
			if(size > (1 << linearThreshold)) {
				uint s = DefaultArrayResizePolicy::standard(2048);
				return s + ((1 << linearThreshold) * (((size - s) >> linearThreshold) + 1));
			}
			return DefaultArrayResizePolicy::standard(size);
		}

		bool shrink(uint s, uint cap) const {
			return s + 2048 < cap;
		}
};

class OptimalArrayResizePolicy
{
	protected:
		uint size(uint size) const {
			return size;
		}

		bool shrink(uint s, uint cap) const {
			return s < cap;
		}
};

template<typename T, typename ResizePolicy = DefaultArrayResizePolicy>
class Array : public ResizePolicy// Be SUPER careful when adding collections directly, will use the lazyest cast possible, can be wrong !
{
	using TT = typename TypeInfo<T>::nonConst;
	public:
		typedef T * iterator;
		typedef T const * const_iterator;
		typedef T Element;

		Array();
		explicit Array(uint s);
		Array(uint s, const T &e);
		Array(const Array<T, ResizePolicy> &a);
		Array(Array<T, ResizePolicy> &&a);

		template<typename C, typename R>
		Array(const Array<C, R> &a);

		template<typename I>
		Array(I b, I e);

		template<typename C>
		Array(std::initializer_list<C> l);

		~Array();





		template<typename C>
		Array<T, ResizePolicy> &append(const C &c);
		template<typename C>
		Array<T, ResizePolicy> &append(std::initializer_list<C> l);


		iterator remove(const T &e);


		iterator erase(const_iterator b);
		iterator erase(const_iterator b, const_iterator e);


		template<typename I>
		iterator insert(I b, I e, iterator position);
		template<typename C>
		iterator insert(const C &e, iterator position);
		template<typename C>
		iterator insert(const C &e);


		template<typename C>
		void assign(const C &c);
		template<typename I>
		void assign(I b, I e);


		template<typename RP>
		void swap(Array<T, RP> &arr);
		template<typename RP>
		void swap(Array<T, RP> &&arr);


		void pop();


		uint size() const;
		uint getCapacity() const;
		bool isEmpty() const;


		void makeEmpty();
		void clear();


		const T &operator[](uint i) const;
		T &operator[](uint i);

		template<typename C>
		bool operator==(const C &l) const;
		template<typename C>
		bool operator!=(const C &c) const;


		template<typename C>
		Array<T, ResizePolicy> &operator+=(const C &e);
		template<typename C>
		Array<T, ResizePolicy> &operator<<(const C &e);


		template<typename C>
		Array<T, ResizePolicy> &operator=(const C &c);
		Array<T, ResizePolicy> &operator=(Array<T, ResizePolicy> &&arr);
		Array<T, ResizePolicy> &operator=(const Array<T, ResizePolicy> &e);


		template<typename C>
		Array<T, ResizePolicy> operator+(const C &e);


		void setCapacity(uint cap);
		void setMinCapacity(uint s);
		void reserve(uint s);
		void squeeze();


		iterator getIterator(uint i);
		const_iterator getIterator(uint i) const;


		bool isValid(const_iterator i) const;


		const_iterator begin() const;
		const_iterator end() const;
		const_iterator cbegin() const;
		const_iterator cend() const;
		iterator begin();
		iterator end();


		T &first();
		T &last();
		const T &first() const;
		const T &last() const;


		template<typename C = std::less<T>>
		void sort(const C &c = C());
		bool isSorted() const;


		template<typename U>
		void foreach(const U &f);
		template<typename U>
		void foreach(const U &f) const;


		template<typename V, typename C = Array<typename std::result_of<V(const T &)>::type, ResizePolicy>>
		C mapped(const V &f) const;
		template<typename V>
		void map(const V &f);


		template<typename U, typename C = Array<T, ResizePolicy>>
		C filtered(const U &f) const;
		template<typename U>
		void filter(const U &f);


		template<typename U>
		bool forall(const U &f) const;



		template<typename U>
		iterator findOne(const U &f, const_iterator from = 0, const_iterator to = 0);
		template<typename U>
		const_iterator findOne(const U &f, const_iterator from = 0, const_iterator to = 0) const;

		iterator find(const T &e, const_iterator from = 0, const_iterator to = 0);
		const_iterator find(const T &e, const_iterator from = 0, const_iterator to = 0) const;


		template<typename U>
		uint countAll(const U &f, const_iterator from = 0, const_iterator to = 0) const;

		uint count(const T &e, const_iterator from = 0, const_iterator to = 0) const;


		template<typename V>
		bool existsOne(const V &f, const_iterator from = 0, const_iterator to = 0) const;

		bool exists(const T &e, const_iterator from = 0, const_iterator to = 0) const;



	private:
		template<typename U, typename R>
		friend class Array;

		void copy(TT *dst, const TT *src, uint n) {
			if(TypeInfo<T>::isPod) {
				memcpy(dst, src, sizeof(T) * n);
			} else {
				for(; n; n--) {
					new(dst++) TT(*(src++));
				}
			}
		}

		void move(TT *dst, const TT *src, uint n) {
			if(TypeInfo<T>::isPod) {
				memmove(dst, src, sizeof(T) * n);
			} else {
				for(; n; n--) {
					new(dst++) TT(std::move(*(src++)));
				}
			}
		}

		void moveBack(TT *dst, uint n, uint interval = 1) {
			n -= interval;
			if(TypeInfo<T>::isPod) {
				memmove(dst, dst + interval, sizeof(T) * n);
			} else {
				for(T *e = dst + n; dst != e; dst++) {
					std::swap(*dst, *(dst + interval));
				}
				for(; interval; interval--) {
					dst->~T();
					dst++;
				}
			}
		}

		void clear(TT *src, uint n) {
			if(!TypeInfo<T>::isPod) {
				for(; n; n--) {
					(src++)->~TT();
				}
			}
		}

		void expend() {
			uint s = size();
			uint ns = ResizePolicy::size(s);
			setCapacityUnsafe(s, ns);
		}

		void shrinkIfNeeded() {
			uint cc = getCapacity();
			uint s = size();
			if(this->shrink(s, cc)) {
				uint tc = ResizePolicy::size(s);
				if(cc != tc) {
					setCapacityUnsafe(s, tc);
				}
			}
		}

		template<typename C>
		iterator insertDispatch(const C &e, iterator position, FalseType) {
			if(getCapacity() - size()) {
				move(position + 1, position, dataEnd - position);
				position->~T();
				new(position) T(e);
				return position + 1;
			}
			return insert(&e, (&e) + 1, position);
		}

		template<typename C>
		iterator insertDispatch(const C &e, iterator position, TrueType) {
			return insert(e.begin(), e.end(), position);
		}

		template<typename C>
		void appendDispatch(const C &e, TrueType) {
			if(dataEnd == allocEnd) {
				expend();
			}
			new(dataEnd++) TT(e);
		}


		template<typename C>
		void appendDispatch(const C &c, FalseType) {
			static_assert(Collection<C>::isCollection, "Can not build n::core::Array<T> from given type. (type is not convertible to T and is not a collection of T)");
			setMinCapacity(size() + c.size());
			for(const auto &e : c) {
				append(e);
			}
		}

		void append() {
		}

		void setCapacityUnsafe(uint s, uint ns) {
			if(TypeInfo<T>::isPod) {
				data = reinterpret_cast<TT *>(safeRealloc(data, ns * sizeof(T)));
			} else {
				TT *n = reinterpret_cast<TT *>(malloc(ns * sizeof(T)));
				move(n, data, s);
				clear(data, s);
				free(data);
				data = n;
			}
			dataEnd = data + s;
			allocEnd = data + ns;
		}

		TT *data;
		TT *dataEnd;
		TT *allocEnd;
};


} //core
} //n


template<typename T>
n::core::Array<T> operator+(const T &i, const n::core::Array<T> &a) {
	return n::core::Array<T>({i}) + a;
}

#include "Array_impl.h"



#endif // N_CORE_ARRAY_H
