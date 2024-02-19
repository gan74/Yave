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
#ifndef YAVE_SCENE_OCTREE_H
#define YAVE_SCENE_OCTREE_H

#include "OctreeNode.h"

namespace yave {

class Octree : NonMovable {

    public:
        Octree();

        OctreeNode* insert(ecs::EntityId id, const AABB& bbox);

        const OctreeNode& root() const;

        core::Vector<ecs::EntityId> all_entities() const;
        core::Vector<ecs::EntityId> find_entities(const Frustum& frustum, float far_dist = -1.0f) const;

    private:
        friend class OctreeSystem;

        OctreeData _data;
        std::unique_ptr<OctreeNode> _root;

        void audit() const;
};

}

#endif // YAVE_SCENE_OCTREE_H

