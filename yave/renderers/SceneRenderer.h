/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_RENDERERS_SCENERENDERERNODE_H
#define YAVE_RENDERERS_SCENERENDERERNODE_H

#include <yave/pipeline/CullingNode.h>

namespace yave {

class SceneRenderer {

	public:
		SceneRenderer(DevicePtr dptr, const Node::Ptr<CullingNode>& cull);

		const SceneView& scene_view() const;

		core::Vector<Node::NodePtr> dependencies();
		void process(const FrameToken&, CmdBufferRecorder<>& recorder);

	private:
		void setup_instance(CmdBufferRecorder<>& recorder, const AssetPtr<StaticMeshInstance>& instance);
		void submit_batches(CmdBufferRecorder<>& recorder, AssetPtr<Material>& mat, usize offset, usize size);

		Node::Ptr<CullingNode> _cull;

		TypedBuffer<uniform::ViewProj, BufferUsage::UniformBit> _camera_buffer;
		TypedMapping<uniform::ViewProj, MemoryFlags::CpuVisible> _camera_mapping;
		DescriptorSet _camera_set;


		TypedBuffer<math::Matrix4<>, BufferUsage::AttributeBit, MemoryFlags::CpuVisible> _matrix_buffer;
		TypedMapping<math::Matrix4<>, MemoryFlags::CpuVisible> _matrix_mapping;


		TypedBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit, MemoryFlags::CpuVisible> _indirect_buffer;
		TypedMapping<vk::DrawIndexedIndirectCommand, MemoryFlags::CpuVisible> _indirect_mapping;

};

}

#endif // YAVE_RENDERERS_SCENERENDERERNODE_H
