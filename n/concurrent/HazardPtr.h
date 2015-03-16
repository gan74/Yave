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

#ifndef N_CONCURENT_HAZARDPTR
#define N_CONCURENT_HAZARDPTR

#include "SpinLock.h"
#include <n/core/Array.h>

namespace n {
namespace concurrent {
namespace internal {

	void registerHazard(const void **ptr);
	void unregisterHazard(const void **ptr);
	void closeThreadHazards();
	core::Array<const void **> getHazards();
	uint getHazardCount();
	void deleteThreadHazards();

	struct DeleteNode;
	struct DeleteList;
	struct HazardPtr;

	extern thread_local HazardPtr *hazard;
	extern thread_local core::Array<DeleteNode *> *deleteList;

	struct HazardPtr : core::NonCopyable
	{
		HazardPtr() : ptr(0) {
			registerHazard(&ptr);
		}

		~HazardPtr() {
			unregisterHazard(&ptr);
		}

		const void *ptr;
	};

	struct DeleteNode : core::NonCopyable
	{
		DeleteNode(const void *p) : ptr(p) {
			if(!deleteList) {
				deleteList = new core::Array<DeleteNode *>();
			}
			deleteList->append(this);
			uint max = getHazardCount();
			max = max + (max >> 2);
			if(deleteList->size() > max) {
				deleteThreadHazards();
			}
		}

		virtual ~DeleteNode() {
		}

		virtual void free() = 0;

		const void *ptr;
	};



	template<typename T>
	class DeleteNodeTemplate : public DeleteNode
	{
		public:
			DeleteNodeTemplate(const T *p) : DeleteNode(p) {
			}

			virtual void free() override {
				delete (const T *)ptr;
			}
	};
}

template<typename T>
class HazardPtr : core::NonCopyable
{
	public:
		HazardPtr(const T *p) : ptr(p) {
			if(!internal::hazard) {
				internal::hazard = new internal::HazardPtr();
			}
			if(internal::hazard->ptr) {
				fatal("Only one HazardPtr is allowed per thread !");
			}
			internal::hazard->ptr = ptr;
		}

		HazardPtr(HazardPtr<T> &&p) : HazardPtr(p.ptr) {
			p.ptr = 0;
		}

		~HazardPtr() {
			if(internal::hazard->ptr == ptr) {
				internal::hazard->ptr = 0;
			}
		}

		const T &operator*() const {
			return *ptr;
		}

		const T *operator->() const {
			return ptr;
		}

		operator const T *() const {
			return ptr;
		}

		static void deleteLater(T *ptr) {
			new internal::DeleteNodeTemplate<T>(ptr);
		}

	private:
		const T *ptr;
};

}
}


#endif // N_CONCURENT_HAZARDPTR

