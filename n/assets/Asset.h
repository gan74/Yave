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

namespace n {
namespace assets {

template<typename T>
class AssetPtr : public core::SmartPtr<concurrent::AtomicAssignable<const T *>, concurrent::auint>
{
	public:
		using PtrType = concurrent::AtomicAssignable<const T *>;
		typedef typename core::SmartPtr<PtrType, concurrent::auint> InternalType;

	private:
		static const InternalType invalidPtr;

	public:

		AssetPtr(PtrType *a = 0) : InternalType(std::move(a)) {
		}

		bool isValid() const {
			return (*this) != invalidPtr;
		}

		void invalidate() const {
			if(isValid()) {
				const_cast<AssetPtr<T> *>(this)->InternalType::operator=(invalidPtr);
			}
		}
};

template<typename T, typename LoadPolicy>
class AssetManager;

template<typename T>
class Asset
{
	public:
		Asset() : ptr(0) {
		}

		Asset(T *&&p) : ptr(AssetPtr<T>(new typename AssetPtr<T>::PtrType(p))) {
		}

		~Asset() {
			if(ptr.getReferenceCount() < 1) {
				ptr.invalidate();
			}
		}

		const T *operator->() const {
			return ptr ? (const T *)(*ptr) : 0;
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

		bool operator==(const Asset<T> &a) const {
			return ptr == a.ptr;
		}

		bool operator!=(const Asset<T> &a) const {
			return !operator==(a);
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
