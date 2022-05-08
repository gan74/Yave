/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "MeshDrawData.h"

namespace yave {

bool MeshDrawData::is_null() const {
    return _triangle_buffer.is_null();
}

TriangleSubBuffer MeshDrawData::triangle_buffer() const {
    return _triangle_buffer;
}

std::array<AttribSubBuffer, 3> MeshDrawData::attrib_buffers() const {
    return std::array<AttribSubBuffer, 3> {
        _attrib_buffers.positions,
        _attrib_buffers.normals_tangents,
        _attrib_buffers.uvs
    };
}

const TypedAttribSubBuffer<math::Vec3>& MeshDrawData::position_buffer() const {
    return _attrib_buffers.positions;
}

const VkDrawIndexedIndirectCommand& MeshDrawData::indirect_data() const {
    return _indirect_data;
}

}


