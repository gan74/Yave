/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_MESHES_SKINNEDMESH_H
#define YAVE_MESHES_SKINNEDMESH_H

#include "Skeleton.h"

#include <yave/graphics/buffers/buffers.h>

#include <yave/assets/AssetTraits.h>

namespace yave {

class SkinnedMesh : NonCopyable {

    public:
        SkinnedMesh() = default;

        SkinnedMesh(const MeshData& mesh_data);

        const TriangleBuffer<>& triangle_buffer() const;
        const SkinnedVertexBuffer<>& vertex_buffer() const;

        const VkDrawIndexedIndirectCommand& indirect_data() const;
        const Skeleton& skeleton() const;

        float radius() const;

    private:
        TriangleBuffer<> _triangle_buffer;
        SkinnedVertexBuffer<> _vertex_buffer;
        VkDrawIndexedIndirectCommand _indirect_data = {};

        Skeleton _skeleton;

        float _radius;
};

YAVE_DECLARE_GRAPHIC_ASSET_TRAITS(SkinnedMesh, MeshData, AssetType::Mesh);

}

#endif // YAVE_MESHES_SKINNEDMESH_H

