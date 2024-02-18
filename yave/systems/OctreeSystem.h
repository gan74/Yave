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
#ifndef YAVE_SYSTEMS_OCTREESYSTEM_H
#define YAVE_SYSTEMS_OCTREESYSTEM_H

#include <yave/ecs/System.h>

#include <yave/scene/Octree.h>

#include <y/concurrent/Signal.h>
#include <y/core/Vector.h>

namespace yave {

class OctreeSystem : public ecs::System {
    public:
        OctreeSystem();

        void destroy() override;
        void setup() override;
        void tick() override;

        core::Vector<ecs::EntityId> find_entities(const Camera& camera) const;

        const OctreeNode& root() const;

    private:
        void run_tick(bool only_recent);

        Octree _tree;

        concurrent::Subscription _transform_destroyed;
};

}

#endif // YAVE_SYSTEMS_OCTREESYSTEM_H

