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
#ifndef YAVE_SCENE_SCENE_H
#define YAVE_SCENE_SCENE_H

#include "TransformManager.h"

#include <yave/systems/RendererSystem.h>

#include <yave/camera/Camera.h>
#include <yave/assets/AssetPtr.h>

#include <y/core/Vector.h>
#include <y/core/PagedSet.h>
#include <y/core/FixedArray.h>

namespace yave {

struct SceneStaticMesh {
    u32 transform_index = u32(-1);
    u32 entity_index = u32(-1);

    bool hidden = false;

    AssetPtr<StaticMesh> mesh;
    core::Vector<AssetPtr<Material>> materials;
};

class Scene : NonMovable {
    static constexpr BufferUsage buffer_usage = BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit;
    using TransformBuffer = TypedBuffer<shader::TransformableData, buffer_usage>;

    public:
        using RenderFunc = std::function<void(RenderPassRecorder& render_pass, const FrameGraphPass* pass)>;

        Scene() = default;

        virtual ~Scene();

        core::Vector<const SceneStaticMesh*> gather_visible_meshes(const Camera& cam) const;

        RenderFunc prepare_render(FrameGraphPassBuilder& builder, const Camera& cam,  PassType pass_type);

    protected:
        TransformManager _transform_manager;

        core::PagedSet<SceneStaticMesh> _meshes;
};

}

#endif // YAVE_SCENE_SCENE_H

