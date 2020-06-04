/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_ECS_ENTITYIDPOOL_H
#define Y_ECS_ENTITYIDPOOL_H

#include "ecs.h"

#include <y/core/Vector.h>
#include <y/core/Range.h>

#include <y/serde3/serde.h>

#include <y/utils/iter.h>

namespace y {
namespace ecs {

class EntityIDPool {
	public:
		usize size() const;
		bool contains(EntityID id) const;

		EntityID id_from_index(u32 index) const;

		EntityID create();
		void recycle(EntityID id);


		auto ids() const {
			return core::Range(
				FilterIterator(_ids.begin(), _ids.end(), [](EntityID id) { return id.is_valid(); }),
				EndIterator()
			);
		}

		y_serde3(_ids, _free)

	private:
		core::Vector<EntityID> _ids;
		core::Vector<u32> _free;
};

}
}

#endif // Y_ECS_ENTITYIDPOOL_H
