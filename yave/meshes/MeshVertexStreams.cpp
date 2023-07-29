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

#include "MeshVertexStreams.h"


namespace yave {

MeshVertexStreams::MeshVertexStreams(usize vertices) : _vertex_count(vertices) {
    usize offset = 0;
    for(usize i = 0; i != stream_count; ++i) {
        const usize stream_elem_size = vertex_stream_element_size(VertexStreamType(i));
        const usize stream_byte_size = _vertex_count * stream_elem_size;
        _stream_offsets[i] = offset;
        offset += stream_byte_size;
    }
    _storage = core::FixedArray<u8>(offset);
}

MeshVertexStreams::MeshVertexStreams(core::Span<PackedVertex> vertices) : MeshVertexStreams(vertices.size()) {
    for(usize i = 0; i != _vertex_count; ++i) {
        set_vertex(i, vertices[i]);
    }
}

MeshVertexStreams MeshVertexStreams::merged(const MeshVertexStreams& other) const {
    MeshVertexStreams str(vertex_count() + other.vertex_count());

    for(usize i = 0; i != stream_count; ++i) {
        const VertexStreamType type = VertexStreamType(i);
        const usize stream_elem_size = vertex_stream_element_size(type);

        const usize stream_total_size = vertex_count() * stream_elem_size;
        std::memcpy(str.data(type), data(type), stream_total_size);
        std::memcpy(str.data(type) + stream_total_size, other.data(type), other.vertex_count() * stream_elem_size);
    }

    return str;
}

PackedVertex MeshVertexStreams::operator[](usize index) const {
    y_debug_assert(index < _vertex_count);

    PackedVertex vertex;
    u8* vertex_data = reinterpret_cast<u8*>(&vertex);

    usize offset = 0;
    for(usize i = 0; i != stream_count; ++i) {
        const VertexStreamType type = VertexStreamType(i);
        const usize stream_elem_size = vertex_stream_element_size(type);
        std::memcpy(vertex_data + offset, vertex_stream_data(type, index), stream_elem_size);
        offset += stream_elem_size;
    }

    return vertex;
}

void MeshVertexStreams::set_vertex(usize index, PackedVertex vertex) {
    y_debug_assert(index < _vertex_count);

    const u8* vertex_data = reinterpret_cast<const u8*>(&vertex);

    usize offset = 0;
    for(usize i = 0; i != stream_count; ++i) {
        const VertexStreamType type = VertexStreamType(i);
        const usize stream_elem_size = vertex_stream_element_size(type);
        std::memcpy(vertex_stream_data(type, index), vertex_data + offset, stream_elem_size);
        offset += stream_elem_size;
    }
}

usize MeshVertexStreams::vertex_count() const {
    return _vertex_count;
}

bool MeshVertexStreams::is_empty() const {
    return !_vertex_count;
}

bool MeshVertexStreams::has_stream(VertexStreamType type) const {
    return data(type);
}

u8* MeshVertexStreams::data(VertexStreamType type) {
    const usize offset = _stream_offsets[usize(type)];
    return _storage.data() + offset;
}

const u8* MeshVertexStreams::data(VertexStreamType type) const {
    const usize offset = _stream_offsets[usize(type)];
    return _storage.data() + offset;
}

}

