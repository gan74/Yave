/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_MESHES_STATICMESH_H
#define YAVE_MESHES_STATICMESH_H

#include "AABB.h"
#include "MeshDrawData.h"

#include <yave/assets/AssetTraits.h>

#include <y/core/FixedArray.h>

Y_TODO(move into graphics?)

namespace yave {

class StaticMesh : NonCopyable {
    public:
        StaticMesh() = default;
        StaticMesh(const MeshData& mesh_data);

        StaticMesh(StaticMesh&&) = default;
        StaticMesh& operator=(StaticMesh&&) = default;

        ~StaticMesh();

        bool is_null() const;

        const MeshDrawData& draw_data() const;
        const MeshDrawCommand& draw_command() const;
        const core::Span<MeshDrawCommand> sub_meshes() const;

        float radius() const;
        const AABB& aabb() const;

    private:
        MeshDrawData _draw_data = {};
        core::FixedArray<MeshDrawCommand> _sub_meshes;
        AABB _aabb;
};

YAVE_DECLARE_GRAPHIC_ASSET_TRAITS(StaticMesh, MeshData, AssetType::Mesh);

}

#endif // YAVE_MESHES_STATICMESH_H

