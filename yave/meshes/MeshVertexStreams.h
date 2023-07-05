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
#ifndef YAVE_MESHES_MESH_VERTEX_STREAMS_H
#define YAVE_MESHES_MESH_VERTEX_STREAMS_H

#include "Vertex.h"

#include <y/core/FixedArray.h>
#include <y/core/Span.h>
#include <memory>

namespace yave {

struct AttribDescriptor {
    usize size;
};

enum class VertexStreamType : u32 {
    Position = 0,
    NormalTangent,
    Uv,

    Max
};

constexpr usize vertex_stream_element_size(VertexStreamType type) {
    switch(type) {
        case VertexStreamType::Position:
            return sizeof(PackedVertex::position);

        case VertexStreamType::NormalTangent:
            return sizeof(PackedVertex::packed_normal) + sizeof(PackedVertex::packed_tangent_sign);

        case VertexStreamType::Uv:
            return sizeof(PackedVertex::uv);

        default:
            y_fatal("Unknown vertex stream type");
    }
}

constexpr usize compute_total_vertex_streams_size() {
    usize total_size = 0;
    for(usize i = 0; i != usize(VertexStreamType::Max); ++i) {
        total_size += vertex_stream_element_size(VertexStreamType(i));
    }
    return total_size;
}

class MeshVertexStreams {
    public:
        static constexpr usize stream_count = usize(VertexStreamType::Max);
        static constexpr usize total_vertex_size = compute_total_vertex_streams_size();  Y_TODO(rename)

        MeshVertexStreams() = default;

        MeshVertexStreams(core::Span<PackedVertex> vertices) : _vertex_count(vertices.size()) {
            usize offset = 0;
            for(usize i = 0; i != stream_count; ++i) {
                const usize stream_elem_size = vertex_stream_element_size(VertexStreamType(i));
                const usize stream_byte_size = _vertex_count * stream_elem_size;
                _streams[i] = core::FixedArray<u8>(stream_byte_size);

                u8* stream_data = _streams[i].data();
                for(usize k = 0; k != _vertex_count; ++k) {
                    const u8* vertex = static_cast<const u8*>(static_cast<const void*>(&vertices[k]));
                    std::memcpy(stream_data + k * stream_elem_size, vertex + offset, stream_elem_size);
                }

                offset += stream_elem_size;
            }
        }

        bool has_stream(VertexStreamType type) const {
            return !_streams[usize(type)].is_empty();
        }

        usize vertex_count() const {
            return _vertex_count;
        }

        const u8* data(VertexStreamType type) const {
            return _streams[usize(type)].data();
        }

    private:
        usize _vertex_count = 0;
        std::array<core::FixedArray<u8>, stream_count> _streams;

};

}

#endif // YAVE_MESHES_MESH_VERTEX_STREAMS_H

