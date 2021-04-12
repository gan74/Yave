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
#ifndef YAVE_SCENE_OCTREEDATA_H
#define YAVE_SCENE_OCTREEDATA_H

#include <yave/yave.h>

#include <y/core/Vector.h>

namespace yave {

class OctreeEntityId {
    public:
        OctreeEntityId() = default;

        bool is_valid() const;

        bool operator==(const OctreeEntityId& other) const;

    private:
        friend class OctreeData;

        static constexpr u64 invalid_id = u64(-1);

        OctreeEntityId(u64 id);
        u64 _id = invalid_id;
};

class OctreeData : NonMovable {
    public:
        OctreeData() = default;

        OctreeEntityId create_id();

        void set_dirty(OctreeEntityId id);

    private:
        core::Vector<OctreeEntityId> _dirty;
        u64 _next_id = 0;
};


}

#endif // YAVE_SCENE_OCTREEDATA_H

