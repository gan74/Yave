/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef EDITOR_RENDERERS_IMGUIRENDERER_H
#define EDITOR_RENDERERS_IMGUIRENDERER_H

#include <editor/editor.h>

#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/buffers/MultiBufferWrapper.h>
#include <yave/material/Material.h>

namespace editor {

class ImGuiRenderer : public DeviceLinked, NonCopyable {

	struct Vertex {
		math::Vec2 pos;
		math::Vec2 uv;
		u32 col;
	};

	public:
		ImGuiRenderer(DevicePtr dptr);

		void render(RenderPassRecorder& recorder, const FrameToken& token);

	protected:
		void setup_state(RenderPassRecorder& recorder, const FrameToken& token, const void* data);

	private:
		const DescriptorSet& create_descriptor_set(const void* tex);

		MultiBufferWrapper<TypedBuffer<u32, BufferUsage::IndexBit, MemoryType::CpuVisible>> _index_buffer;
		MultiBufferWrapper<TypedBuffer<Vertex, BufferUsage::AttributeBit, MemoryType::CpuVisible>> _vertex_buffer;
		TypedUniformBuffer<math::Vec2> _uniform_buffer;

		Texture _font;
		TextureView _font_view;
		Material _material;

		std::unordered_map<VkImageView, DescriptorSet> _descriptor_sets;

};

}

#endif // EDITOR_RENDERERS_IMGUIRENDERER_H
