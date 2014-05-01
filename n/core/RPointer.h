/*******************************
Copyright (T) 2009-2010 gr�goire ANGERAND

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

#ifndef N_CORE_RPOINTER_H
#define N_CORE_RPOINTER_H

#include <n/Types.h>
#include <functional>
#include "Option.h"

namespace n {
namespace core {

template<typename T>
class RPointer
{
	public:
		RPointer(T *&& p = 0) : data(p), count(new uint(1)) {
		}

		RPointer(const RPointer &rp) {
			ref(rp);
		}

		~RPointer() {
			unref();
		}

		bool isNull() const {
			return !data;
		}

		uint getReferenceCount() const {
			return *count;
		}

		T& operator*() const {
			return *data;
		}

		T* operator->() const {
			return data;
		}

		RPointer<T> &operator=(const RPointer<T> &rp) {
			if(rp.count != count) {
				unref();
				ref(rp);
			}
			return *this;
		}

		RPointer<T> &operator=(T *&& p) {
			unref();
			data = p;
			count = new uint(1);
			return *this;
		}

		operator RPointer<const T>() const {
			return RPointer<const T>(data, count);
		}

		bool operator!() const {
			return !data;
		}

		bool operator==(const RPointer &p) const {
			return p.data == data;
		}

		bool operator!=(const RPointer &p) const {
			return p.data != data;
		}

		bool operator==(T *p) const {
			return p == data;
		}

		bool operator!=(T *p) const {
			return p != data;
		}

		operator bool() const {
			return !isNull();
		}

		/*RPointer<typename TypeInfo<T>::nonConst> detach() {
			if(*count != 1) {
				unref();
				count = new uint(1);
				data = new T(*data);
			}
			return RPointer<typename TypeInfo<T>::nonConst>(const_cast<typename TypeInfo<T>::nonConst *>(data), count);
		}*/

	private:
		friend class RPointer<const T>;
		friend class RPointer<typename TypeInfo<T>::nonConst>; // what ?

		RPointer(T *p, uint *c) : data(p), count(c) {
			(*count)++;
		}

		void ref(const RPointer<T> &rp) {
			data = rp.data;
			count = rp.count;
			(*count)++;
		}

		void unref() {
			if(!--(*count)) {
				delete data;
				delete count;
			}
		}

		T *data;
		uint *count;
};



template<typename T>
class SRPointer;

template<typename T>
class WRPointer
{
	public:
		WRPointer(const WRPointer<T> &o) : data(o.data), count(o.count), weak(o.weak), onDel(o.onDel) {
			(*weak)++;
		}

		~WRPointer() {
			if(!--(*weak)) {
				delete count;
				delete weak;
				delete onDel;
			}
		}

		void setOnDelete(const std::function<void(const T *)> &f) {
			onDel->set(f);
		}

		void addOnDelete(const std::function<void(const T *)> &f) {
			if(onDel->hasValue()) {
				std::function<void(const T *)> fu = onDel->getValue();
				setOnDelete([=](const T *t) { fu(t); f(t); });
			} else {
				setOnDelete(f);
			}
		}

		bool isValid() const {
			return *count;
		}

		bool isNull() const {
			return !data || !isValid();
		}

		uint getReferenceCount() const {
			return *count;
		}

		WRPointer<T> &operator=(const WRPointer<T> &o) {
			data = o.data;
			count = o.count;
			weak = o.weak;
			onDel = o.onDel;
			(*weak)++;
			return *this;
		}

		operator WRPointer<const T>() const {
			return WRPointer<const T>(data, count, weak, onDel);
		}

		T& operator*() const {
			return *data;
		}

		T* operator->() const {
			return data;
		}

		bool operator!() const {
			return !data;
		}

		bool operator==(const WRPointer &p) const {
			return p.data == data;
		}

		bool operator!=(const WRPointer &p) const {
			return p.data != data;
		}

		bool operator==(T *p) const {
			return p == data;
		}

		bool operator!=(T *p) const {
			return p != data;
		}

		operator bool() const {
			return !isNull();
		}

	private:
		friend class SRPointer<T>;
		WRPointer(T *p, uint *c, uint *w, Option<std::function<void(const T *)>> *d) : data(p), count(c), weak(w), onDel(d) {
			(*weak)++;
		}

		T *data;
		uint *count;
		uint *weak;
		Option<std::function<void(const T *)>> *onDel; // for casting to s-pointer
};

template<typename T>
class SRPointer
{
	public:
		SRPointer(T *&& p = 0) : data(p), count(new uint(1)), weak(new uint(0)), onDel(new Option<std::function<void(const T *)>>()) {
		}

		SRPointer(const WRPointer<T> &rp) : SRPointer() {
			if(*rp.count) {
				ref(rp);
			}
		}

		SRPointer(const SRPointer &rp) {
			ref(rp);
		}

		~SRPointer() {
			unref();
		}

		void setOnDelete(const std::function<void(const T *)> &f) {
			onDel->set(f);
		}

		void addOnDelete(const std::function<void(const T *)> &f) {
			if(onDel->hasValue()) {
				std::function<void(const T *)> fu = onDel->getValue();
				setOnDelete([=](const T *t) { fu(t); f(t); });
			} else {
				setOnDelete(f);
			}
		}

		bool isNull() const {
			return !data;
		}

		uint getReferenceCount() const {
			return *count;
		}

		T& operator*() const {
			return *data;
		}

		T* operator->() const {
			return data;
		}

		SRPointer<T> &operator=(const SRPointer<T> &rp) {
			if(rp.count != count) {
				unref();
				ref(rp);
			}
			return *this;
		}

		SRPointer<T> &operator=(T *&& p) {
			unref();
			data = p;
			count = new uint(1);
			weak = new uint(0);
			onDel = new Option<std::function<void(const T *)>>();
			return *this;
		}

		operator SRPointer<const T>() const {
			return SRPointer<const T>(data, count, weak, onDel);
		}

		bool operator!() const {
			return !data;
		}

		bool operator==(const SRPointer &p) const {
			return p.data == data;
		}

		bool operator!=(const SRPointer &p) const {
			return p.data != data;
		}

		bool operator==(T *p) const {
			return p == data;
		}

		bool operator!=(T *p) const {
			return p != data;
		}

		operator bool() const {
			return !isNull();
		}

		SRPointer<typename TypeInfo<T>::nonConst> detach() {
			if(*count != 1) {
				unref();
				count = new uint(1);
				weak = new uint(0);
				data = new T(*data);
				onDel = new Option<std::function<void()>>();
			}
			return SRPointer<typename TypeInfo<T>::nonConst>(const_cast<typename TypeInfo<T>::nonConst *>(data), count, weak, onDel);
		}

		WRPointer<T> getWeakPointer() const {
			return WRPointer<T>(data, count, weak, onDel);
		}

		operator WRPointer<T> () const {
			return getWeakPointer();
		}

	private:
		friend class SRPointer<const T>;
		friend class SRPointer<typename TypeInfo<T>::nonConst>; // what ?

		SRPointer(T *p, uint *c, uint *w, Option<std::function<void(const T *)>> *d) : data(p), count(c), weak(w), onDel(d) {
			(*count)++;
		}

		template<typename W>
		void ref(const W &rp) {
			data = rp.data;
			count = rp.count;
			weak = rp.weak;
			onDel = rp.onDel;
			(*count)++;
		}

		void unref() {
			if(!--(*count)) {
				if(data) {
					if(onDel->hasValue()) {
						onDel->getValue()(data);
					}
					delete data;
					data = 0;
				}
				if(*weak == 0) {
					delete count;
					delete weak;
					delete onDel;
				}
			}
		}

		T *data;
		uint *count;
		uint *weak;
		Option<std::function<void(const T *)>> *onDel;
};

} //core
} //n


#endif // N_CORE_RPOINTER_H
