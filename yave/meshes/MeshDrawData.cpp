/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <yave/graphics/device/MeshAllocator.h>

namespace yave {

core::Span<AttribSubBuffer> MeshDrawBuffers::attrib_buffers() const {
    return _attrib_buffers;
}

usize MeshDrawBuffers::vertex_count() const {
    return _vertex_count;
}

TriangleSubBuffer MeshDrawBuffers::triangle_buffer() const {
    return _triangle_buffer;
}

MeshAllocator* MeshDrawBuffers::parent() const {
    return _parent;
}



MeshDrawData::MeshDrawData(MeshDrawData&& other) {
    swap(other);
}

MeshDrawData& MeshDrawData::operator=(MeshDrawData&& other) {
    swap(other);
    return *this;
}

MeshDrawData::~MeshDrawData() {
    y_debug_assert(is_null());
}

void MeshDrawData::recycle() {
    y_debug_assert(_mesh_buffers);
    _mesh_buffers->parent()->recycle(this);
}

bool MeshDrawData::is_null() const {
    return !_mesh_buffers;
}

TriangleSubBuffer MeshDrawData::triangle_buffer() const {
    y_debug_assert(_mesh_buffers);
    return _mesh_buffers->triangle_buffer();
}

const MeshDrawBuffers& MeshDrawData::mesh_buffers() const {
    y_debug_assert(_mesh_buffers);
    return *_mesh_buffers;
}

const MeshDrawCommand& MeshDrawData::draw_command() const {
    return _command;
}

void MeshDrawData::swap(MeshDrawData& other) {
    std::swap(_command, other._command);
    std::swap(_vertex_count, other._vertex_count);
    std::swap(_mesh_buffers, other._mesh_buffers);
}

}


