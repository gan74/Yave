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
#ifndef YAVE_SYSTEMS_STATICMESHMANAGERSYSTEM_H
#define YAVE_SYSTEMS_STATICMESHMANAGERSYSTEM_H

#include <yave/ecs/System.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/meshes/MeshDrawData.h>

#include <y/core/Vector.h>

namespace yave {

class StaticMeshManagerSystem : public ecs::System {
    public:
        static constexpr usize max_transforms = 2 * 1024;

        struct Batch {
            const MaterialTemplate* material_template = nullptr;
            u32 material_index = u32(-1);
            u32 transform_index = u32(-1);
            MeshDrawCommand draw_cmd = {};
            ecs::EntityId id;
        };


        class RenderList : NonCopyable {
            public:
                void draw(RenderPassRecorder& recorder) const;

                core::Span<Batch> batches() const;

            private:
                friend class StaticMeshManagerSystem;

                core::Vector<Batch> _batches;
                const StaticMeshManagerSystem* _parent = nullptr;
        };



        StaticMeshManagerSystem();

        RenderList create_render_list(core::Span<ecs::EntityId> ids) const;

        void destroy() override;
        void setup() override;
        void tick() override;

        TypedSubBuffer<math::Transform<>, BufferUsage::StorageBit> transform_buffer() const {
            return _transforms;
        }


    private:
        void run_tick(bool only_recent);

        TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::CpuVisible> _transforms;

        core::Vector<u32> _free;
};

}

#endif // YAVE_SYSTEMS_STATICMESHMANAGERSYSTEM_H

