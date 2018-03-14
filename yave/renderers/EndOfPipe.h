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
#ifndef  YAVE_RENDERERS_ENDOFPIPE_H
#define  YAVE_RENDERERS_ENDOFPIPE_H

#include "TiledDeferredRenderer.h"

#include <yave/shaders/ComputeProgram.h>
#include <yave/bindings/DescriptorSet.h>

namespace yave {
namespace experimental {

class EndOfPipe : public Renderer {
	public:
		EndOfPipe(const Ptr<TiledDeferredRenderer>& renderer);

	protected:
		void build_frame_graph(FrameGraphNode& frame_graph) override;
		void render(CmdBufferRecorder<>& recorder, const FrameToken& token) override;

	private:
		const DescriptorSet& create_descriptor_set(const StorageView& out, const TextureView& in);

		Ptr<TiledDeferredRenderer> _renderer;

		ComputeProgram _correction_program;

		std::unordered_map<VkImageView, DescriptorSet> _output_sets;

};

class ScreenEndOfPipe : public Renderer {
	public:
		ScreenEndOfPipe(const Ptr<SecondaryRenderer>& renderer);

	protected:
		void build_frame_graph(FrameGraphNode& frame_graph) override;
		void render(CmdBufferRecorder<>& recorder, const FrameToken& token) override;

	private:
		const Framebuffer& create_framebuffer(const ColorAttachmentView& out);

		Ptr<SecondaryRenderer> _renderer;

		std::unordered_map<VkImageView, Framebuffer> _output_framebuffers;

};


}
}

#endif //  YAVE_RENDERERS_ENDOFPIPE_H
