/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
