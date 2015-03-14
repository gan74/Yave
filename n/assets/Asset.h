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
class AssetPtr : public core::SmartPtr<const T *, concurent::auint>
{
	static constexpr T *InvalidPtr = (T *)0x1;
	public:
		AssetPtr(const T **a) : core::SmartPtr<const T *, concurent::auint>(std::move(a)) {
		}

		bool isValid() const {
			return this->pointer() && this->operator*() != InvalidPtr;
		}

		void invalidate() const {
			if(isValid()) {
				this->operator*() = InvalidPtr;
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

		Asset(T *&&p) : ptr(AssetPtr<T>(new const T*(p))) {
		}

		~Asset() {
			if(ptr.getReferenceCount() < 1) {
				ptr.invalidate();
			}
		}

		const T *operator->() const {
			return ptr ? *ptr : 0;
		}

		const T &operator*() const {
			return **ptr;
		}

		bool isValid() const {
			return ptr.isValid();
		}

		bool isNull() const {
			return !ptr || !*ptr || !isValid();
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

		Asset(AssetPtr<T> &p) : ptr(p) {
		}

		AssetPtr<T> ptr;
};

}
}

#endif // N_ASSETS_ASSET_H
