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
#ifndef YAVE_OBJECTS_POINTLIGHT_H
#define YAVE_OBJECTS_POINTLIGHT_H

#include <yave/buffer/TypedBuffer.h>
#include <yave/descriptors/DescriptorSet.h>

namespace yave {

class PointLight {


	public:
		struct Data {
			math::Vec3 position;
			float radius;
			math::Vec3 color;
			float padding;
		};

		PointLight(DevicePtr dptr, const math::Vec3& pos, float radius) : _uniform_buffer(dptr, 1), _descriptor_set(dptr, {Binding(_uniform_buffer)}) {
			auto map = _uniform_buffer.map();
			map.begin()->position = pos;
			map.begin()->radius = radius;
			map.begin()->color = math::Vec3(1);
		}

	private:
		TypedBuffer<Data, BufferUsage::UniformBuffer> _uniform_buffer;
		DescriptorSet _descriptor_set;

};

}

#endif // YAVE_OBJECTS_POINTLIGHT_H
