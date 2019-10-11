/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include <yave/framegraph/FrameGraph.h>

namespace editor {

class PickingManager : public ContextLinked {
	struct ReadBackData {
		float depth;
		u32 id;
	};

	using ReadBackBuffer = TypedBuffer<ReadBackData, BufferUsage::StorageBit, MemoryType::CpuVisible>;
	public:
		struct PickingData {
			math::Vec3 world_pos;
			float depth;
			math::Vec2 uv;
			u32 entity_index;

			bool hit() const;
		};

		PickingManager(ContextPtr ctx);

		PickingData pick_sync(const SceneView& scene_view, const math::Vec2& uv, const math::Vec2ui& size = math::Vec2ui(512));

	private:
		ReadBackBuffer _buffer;
};

}

#endif // EDITOR_CONTEXT_PICKINGMANAGER_H
