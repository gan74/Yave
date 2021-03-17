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
#ifndef YAVE_MESHES_MESHDATA_H
#define YAVE_MESHES_MESHDATA_H

#include "Skeleton.h"
#include "AABB.h"

#include <y/reflect/reflect.h>

#include <memory>

namespace yave {

class MeshData {

    public:
        MeshData() = default;
        MeshData(core::Vector<Vertex> vertices, core::Vector<IndexedTriangle> triangles, core::Vector<SkinWeights> skin = {}, core::Vector<Bone> bones = {});

        float radius() const;
        const AABB& aabb() const;

        core::Span<Vertex> vertices() const;
        core::Span<IndexedTriangle> triangles() const;

        core::Span<Bone> bones() const;
        core::Span<SkinWeights> skin() const;
        core::Vector<SkinnedVertex> skinned_vertices() const;

        bool has_skeleton() const;

        y_reflect(_aabb, _vertices, _triangles, _skeleton)

    private:
        struct SkeletonData {
            core::Vector<SkinWeights> skin;
            core::Vector<Bone> bones;

            y_reflect(skin, bones)
        };

        AABB _aabb;

        core::Vector<Vertex> _vertices;
        core::Vector<IndexedTriangle> _triangles;

        std::unique_ptr<SkeletonData> _skeleton;
};

}

#endif // YAVE_MESHES_MESHDATA_H

