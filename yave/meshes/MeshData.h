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
#ifndef YAVE_MESHES_MESHDATA_H
#define YAVE_MESHES_MESHDATA_H

#include "Skeleton.h"
#include "MeshVertexStreams.h"
#include "AABB.h"

#include <y/reflect/reflect.h>

#include <memory>

namespace yave {

class MeshData {
    public:
        struct SubMesh {
            u32 triangle_count = 0;
            u32 first_triangle = 0;
        };

        MeshData() = default;

        MeshData(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles);
        MeshData(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles);
        MeshData(MeshVertexStreams streams, core::Span<IndexedTriangle> triangles);

        void add_sub_mesh(const MeshVertexStreams& streams, core::Span<IndexedTriangle> triangles);
        void add_sub_mesh(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles);
        void add_sub_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles);

        u32 add_vertices_from_streams(const MeshVertexStreams& streams);
        void add_sub_mesh(core::Span<IndexedTriangle> triangles, u32 vertex_offset);

        float radius() const;
        const AABB& aabb() const;

        const MeshVertexStreams& vertex_streams() const;
        core::Span<IndexedTriangle> triangles() const;
        core::Span<SubMesh> sub_meshes() const;

        core::Span<Bone> bones() const;
        core::Span<SkinWeights> skin() const;

        bool has_skeleton() const;

        bool is_empty() const;

        y_reflect(MeshData, _aabb, _vertex_streams, _triangles, _sub_meshes, _skeleton)

    private:
        struct SkeletonData {
            core::Vector<SkinWeights> skin;
            core::Vector<Bone> bones;

            y_reflect(SkeletonData, skin, bones)
        };

        AABB _aabb;

        MeshVertexStreams _vertex_streams;
        core::Vector<IndexedTriangle> _triangles;
        core::Vector<SubMesh> _sub_meshes;

        std::unique_ptr<SkeletonData> _skeleton;
};

}

#endif // YAVE_MESHES_MESHDATA_H

