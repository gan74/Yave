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
#ifndef YAVE_SCENE_OBJECTCONTAINER_H
#define YAVE_SCENE_OBJECTCONTAINER_H

#include <yave/buffer/TypedSubBuffer.h>
#include <yave/objects/PointLight.h>

namespace yave {

template<typename T>
class ObjectContainer {

	public:
		using Data = T::Data;

		ObjectContainer(DevicePtr dptr, core::Vector<T>&& objects) : _objects(std::move(objects)), _buffer(dptr, _objects.size()), _mapping(_buffer) {
			for(usize i = 0; i != _objects.size(); i++) {
				_objects[i].set_storage(_mapping[i]);
			}
		}

		/*void append(std::add_rvalue_reference_t<T> t) {
			t.set_storage(TypedSubBuffer<Data, BufferUsage::UniformBuffer>(_buffer, _objects.size()));
			_objects.append(std::move(t));
		}*/

	private:
		core::Vector<T> _objects;
		TypedBuffer<Data, BufferUsage::UniformBuffer> _buffer;
		TypedMapping<Data, MemoryFlags::CpuVisible> _mapping;
};

}

#endif // YAVE_SCENE_OBJECTCONTAINER_H
