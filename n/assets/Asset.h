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

#ifndef N_ASSETS_ASSET_H
#define N_ASSETS_ASSET_H

#include <n/core/SmartPtr.h>
#include <n/utils.h>
#include <n/defines.h>

//#define N_ASSET_ID

namespace n {
namespace assets {

template<typename T>
class AssetPtrStorage
{
	using ConstPtr = const T *;

	#ifndef N_32BITS
	using PtrContainer = concurrent::Atomic<ConstPtr>;
	#else
	using PtrContainer = ConstPtr;
	#endif

	public:
		AssetPtrStorage(ConstPtr &&a) : ptr(a) {
			#ifdef N_ASSET_ID
			id = core::uniqueId();
			#endif
		}

		~AssetPtrStorage() {
			delete ptr;
		}

		AssetPtrStorage<T> &operator=(ConstPtr t) {
			ptr = t;
			return *this;
		}

		AssetPtrStorage<T> &operator=(ConstPtr t) volatile {
			ptr = t;
			return *this;
		}

		operator ConstPtr() const {
			return ptr;
		}

		#ifdef N_ASSET_ID
		uint getId() const {
			return id;
		}
		#endif

	private:
		PtrContainer ptr;

		#ifdef N_ASSET_ID
		uint id;
		#endif
};

template<typename T>
class AssetPtr : public core::SmartPtr<AssetPtrStorage<T>, concurrent::auint>
{
	public:
		using PtrType = AssetPtrStorage<T> ;
		using InternalType = typename core::SmartPtr<PtrType, concurrent::auint>;

	private:
		static const InternalType invalidPtr;

	public:
		AssetPtr(PtrType *a = 0) : InternalType(std::move(a)) {
		}

		bool isValid() const {
			return (*this) != invalidPtr;
		}

		void invalidate() {
			if(isValid()) {
				InternalType::operator=(invalidPtr);
			}
		}
};

template<typename T, typename LoadPolicy>
class AssetManager;

template<typename T>
class Asset
{
	using ConstPtr = const T *;
	public:
		using AssetType = T;
		
		~Asset() {
		}

		T const *operator->() const {
			return getPtr();
		}

		const T &operator*() const {
			return **ptr;
		}

		bool isValid() const {
			return !ptr || ptr.isValid();
		}

		bool isNull() const {
			return !ptr || !*ptr;
		}

		bool isLoading() const {
			return ptr && !*ptr;
		}

		bool operator==(const Asset<T> &a) const {
			return ptr == a.ptr;
		}

		bool operator!=(const Asset<T> &a) const {
			return !operator==(a);
		}

		T const *getPtr() const {
			return ptr ? (const T *)(*ptr) : 0;
		}

		#ifdef N_ASSET_ID
		uint getId() const {
			return !ptr ? 0 : ptr->getId();
		}
		#endif

	protected:
		Asset() : ptr(0) {
		}

		Asset(ConstPtr &&p) : ptr(AssetPtr<T>(new typename AssetPtr<T>::PtrType(std::move(p)))) {
		}

	private:
		template<typename U, typename P>
		friend class AssetManager;

		Asset(const AssetPtr<T> &p) : ptr(p) {
		}

		AssetPtr<T> ptr;
};


template<typename T>
const typename AssetPtr<T>::InternalType AssetPtr<T>::invalidPtr = AssetPtr<T>::InternalType();



}
}


#endif // N_ASSETS_ASSET_H
