/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_CONTEXT_PICKINGMANAGER_H
#define EDITOR_CONTEXT_PICKINGMANAGER_H

#include <editor/editor.h>
#include <yave/graphics/buffers/TypedWrapper.h>

namespace editor {

class PickingManager : public ContextLinked {
    struct ReadBackData {
        const float depth;
        const u32 id;
    };

    using ReadBackBuffer = TypedBuffer<ReadBackData, BufferUsage::StorageBit, MemoryType::CpuVisible>;

    public:
        struct PickingData {
            const math::Vec3 world_pos;
            const float depth;
            const math::Vec2 uv;
            const u32 entity_index;

            bool hit() const;
        };

        PickingManager(ContextPtr ctx);

        PickingData pick_sync(const SceneView& scene_view, const math::Vec2& uv, const math::Vec2ui& size = math::Vec2ui(512));

    private:
        ReadBackBuffer _buffer;
};

}

#endif // EDITOR_CONTEXT_PICKINGMANAGER_H

