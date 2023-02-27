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
#ifndef YAVE_SYSTEMS_SURFELCACHESYSTEM_H
#define YAVE_SYSTEMS_SURFELCACHESYSTEM_H

#include <yave/ecs/System.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/meshes/Vertex.h>

namespace yave {

class SurfelCacheSystem : public ecs::System {
    public:
        struct InstanceData {
            math::Transform<> model;

            math::Vec3 center;
            float radius;

            math::Vec2 padding_0;
            u32 surfel_count;
            u32 surfel_offset;

        };

        SurfelCacheSystem();

        void tick(ecs::EntityWorld& world) override;

        u32 surfel_count() const;
        u32 instance_count() const;

        const TypedBuffer<Surfel, BufferUsage::StorageBit>& surfel_buffer() const;
        const TypedBuffer<InstanceData, BufferUsage::StorageBit>& instance_buffer() const;

    private:
        TypedBuffer<Surfel, BufferUsage::StorageBit> _surfels;
        TypedBuffer<InstanceData, BufferUsage::StorageBit> _instances;
        u32 _allocated_surfels = 0;
        u32 _allocated_instances = 0;
};

}

#endif // YAVE_SYSTEMS_SURFELCACHESYSTEM_H

