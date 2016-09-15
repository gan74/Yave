/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef YAVE_QUEUE_H
#define YAVE_QUEUE_H

#include "yave.h"
#include "DeviceLinked.h"

namespace yave {

class QueueBase : NonCopyable, public DeviceLinked {

	public:
		vk::Queue get_vk_queue() const;

	protected:
		QueueBase(DevicePtr dptr, vk::Queue queue);

		void swap(QueueBase& other);

	private:
		// the device owns the queue
		NotOwned<vk::Queue> _queue;
};


/*template<QueueFamily Family>
class Queue : public QueueBase {

	public:
		Queue() = default;

		Queue(Queue&& other) {
			QueueBase::swap(other);
		}

		Queue& operator=(Queue&& other) {
			QueueBase::swap(other);
			return *this;
		}

	private:
		friend class Device;

		Queue(DevicePtr dptr, vk::Queue queue) : QueueBase(dptr, queue) {
		}

};*/


}



#endif // YAVE_QUEUE_H
