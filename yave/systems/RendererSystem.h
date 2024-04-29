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
#ifndef YAVE_SYSTEMS_RENDERERSYSTEM_H
#define YAVE_SYSTEMS_RENDERERSYSTEM_H

#include <yave/ecs/System.h>
#include <yave/ecs/SparseComponentSet.h>

#include <yave/framegraph/FrameGraphResourceId.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>

#include <yave/graphics/shader_structs.h>

#include <y/concurrent/Signal.h>
#include <y/core/Vector.h>


namespace yave {

enum class PassType {
    Depth,
    GBuffer,
    Id,
};

class RendererSystem : public ecs::System {
    public:
        static constexpr BufferUsage buffer_usage = BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit;
        using TransformBuffer = TypedBuffer<shader::TransformableData, buffer_usage>;

        using RenderFunc = std::function<void(RenderPassRecorder& render_pass, const FrameGraphPass* pass)>;

        class Renderer : NonMovable {
            public:
                using RenderFunc = RendererSystem::RenderFunc;

                Renderer(core::String name);
                virtual ~Renderer();

                const ecs::EntityWorld& world() const;
                const RendererSystem* parent() const;

                virtual RenderFunc prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType type) const = 0;

            private:
                friend class RendererSystem;

                RendererSystem* _parent = nullptr;
                core::String _name;
        };


        RendererSystem();

        void destroy() override;
        void setup() override;
        void tick() override;

        RenderFunc prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType pass_type) const;


        TypedSubBuffer<shader::TransformableData, BufferUsage::StorageBit> transform_buffer() const  {
            return _transform_buffer;
        }

        template<typename T>
        void register_component_type() {
            auto renderer = std::make_unique<typename T::Renderer>();
            renderer->_parent = this;
            _renderers << std::move(renderer);
        }

        std::unique_ptr<EcsScene> _scene;

    private:
        void update_transform_buffer(bool only_recent);

        TransformBuffer _transform_buffer;
        core::Vector<std::unique_ptr<Renderer>> _renderers;




};

}

#endif // YAVE_SYSTEMS_RENDERERSYSTEM_H

