/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <y/reflect/reflect.h>

#include <memory>

namespace yave {


enum class VertexStreamType : u32 {
    Position = 0,
    NormalTangent,
    Uv,

    Max
};

template<VertexStreamType Type>
struct VertexStreamInfo {};

#define DECLARE_STREAM_INFOS(VST, Type) template<> struct VertexStreamInfo<VertexStreamType::VST> {  using type = Type; }
    DECLARE_STREAM_INFOS(Position, math::Vec3);
    DECLARE_STREAM_INFOS(NormalTangent, math::Vec2ui);
    DECLARE_STREAM_INFOS(Uv, math::Vec2);
#undef DECLARE_STREAM_INFOS


inline constexpr usize vertex_stream_element_size(VertexStreamType type) {
#define STREAM_CASE(Type) case VertexStreamType::Type: return sizeof(VertexStreamInfo<VertexStreamType::Type>::type)
    switch(type) {
        STREAM_CASE(Position);
        STREAM_CASE(NormalTangent);
        STREAM_CASE(Uv);
        default:
            y_fatal("Unknown stream type");
    }
#undef STREAM_CASE
}

inline constexpr usize compute_total_vertex_streams_size() {
    usize total_size = 0;
    for(usize i = 0; i != usize(VertexStreamType::Max); ++i) {
        total_size += vertex_stream_element_size(VertexStreamType(i));
    }
    return total_size;
}

class MeshVertexStreams {
    public:
        static constexpr usize stream_count = usize(VertexStreamType::Max);
        static constexpr usize total_vertex_size = compute_total_vertex_streams_size();

        MeshVertexStreams() = default;

        MeshVertexStreams(usize vertices);
        MeshVertexStreams(core::Span<PackedVertex> vertices);

        MeshVertexStreams merged(const MeshVertexStreams& other) const;

        usize vertex_count() const;

        bool is_empty() const;

        u8* data(VertexStreamType type);
        const u8* data(VertexStreamType type) const;

        PackedVertex operator[](usize index) const;

        template<VertexStreamType Type>
        inline auto stream() {
            using T = typename VertexStreamInfo<Type>::type;
            return core::MutableSpan<T>(reinterpret_cast<T*>(data(Type)), _vertex_count);
        }

        template<VertexStreamType Type>
        inline auto stream() const {
            using T = typename VertexStreamInfo<Type>::type;
            return core::Span<T>(reinterpret_cast<const T*>(data(Type)), _vertex_count);
        }

        inline void* vertex_stream_data(VertexStreamType type, usize index) {
            return data(type) + index * vertex_stream_element_size(type);
        }

        inline const void* vertex_stream_data(VertexStreamType type, usize index) const {
            return data(type) + index * vertex_stream_element_size(type);
        }


        y_reflect(MeshVertexStreams, _vertex_count, _stream_offsets, _storage);

    private:
        void set_vertex(usize index, PackedVertex vertex);

        usize _vertex_count = 0;
        std::array<usize, stream_count> _stream_offsets;
        core::FixedArray<u8> _storage;

};

}

#endif // YAVE_MESHES_MESH_VERTEX_STREAMS_H

