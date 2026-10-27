/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef YAVE_SCENE_RENDERERS_H
#define YAVE_SCENE_RENDERERS_H

#include <yave/framegraph/FrameGraphResourceId.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/shader_structs.h>
#include <yave/material/Material.h>

#include <functional>

namespace yave {

struct RenderContext {
    const Scene* scene = nullptr;
    FrameGraphTypedBufferId<shader::Camera> camera;
    PassType pass_type = PassType::Depth;
    i32 descriptor_set_index = -1;
};

using RenderFunc = std::function<void(RenderPassRecorder&, const FrameGraphPass*)>;

class RendererBase : NonMovable {
    public:
        virtual ~RendererBase() = default;

        virtual void update(ComputeCapableCmdBufferRecorder&, Scene&) {};

        virtual RenderFunc prepare(FrameGraphPassBuilder& builder, const RenderContext& ctx, const SceneVisibility& visible) const = 0;
};

class StaticMeshRenderer final : public RendererBase {
    public:
        RenderFunc prepare(FrameGraphPassBuilder& builder, const RenderContext& ctx, const SceneVisibility& visible) const override;
};

class SkinnedMeshRenderer final : public RendererBase {
    public:
        using PositionBuffer = Buffer<BufferUsage::StorageBit | BufferUsage::AccelStructureInputBit, MemoryType::DeviceLocal>;
        using MeshDataBuffer = TypedBuffer<shader::StaticMeshData, BufferUsage::StorageBit, MemoryType::CpuVisible>;

        void update(ComputeCapableCmdBufferRecorder& recorder, Scene& scene) override;

        RenderFunc prepare(FrameGraphPassBuilder& builder, const RenderContext& ctx, const SceneVisibility& visible) const override;

    private:
        MeshDataBuffer _mesh_datas;
};

}

#endif // YAVE_SCENE_RENDERERS_H

