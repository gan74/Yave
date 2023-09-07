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
#ifndef YAVE_SYSTEMS_STATICMESHRENDERERSYSTEM_H
#define YAVE_SYSTEMS_STATICMESHRENDERERSYSTEM_H

#include <yave/ecs/System.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/meshes/MeshDrawData.h>
#include <yave/ecs/SparseComponentSet.h>

#include <y/core/Vector.h>

#include <yave/framegraph/FrameGraphResourceId.h>

namespace yave {

class StaticMeshRendererSystem : public ecs::System {
    public:
        static constexpr usize default_buffer_size = 1024;

        static constexpr BufferUsage buffer_usage = BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit;
        using TransformBuffer = TypedBuffer<math::Transform<>, buffer_usage>;

        StaticMeshRendererSystem();

        void destroy() override;
        void setup() override;
        void tick() override;

        TypedSubBuffer<math::Transform<>, BufferUsage::StorageBit> transform_buffer() const {
            return _transforms;
        }


    private:
        TransformBuffer alloc_buffer(usize size);

        void free_index(u32& index);
        void alloc_index(u32& index);

        void run_tick(bool only_recent);

        TransformBuffer _transforms;

        core::Vector<u32> _free;

        ecs::SparseIdSet _moved;
        ecs::SparseIdSet _prev_moved;
};

}

#endif // YAVE_SYSTEMS_STATICMESHRENDERERSYSTEM_H

