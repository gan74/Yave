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
#ifndef YAVE_GRAPHICS_BARRIERS_PIPELINESTAGE_H
#define YAVE_GRAPHICS_BARRIERS_PIPELINESTAGE_H

#include <yave/graphics/vk/vk.h>

namespace yave {

// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineStageFlagBits.html
enum class PipelineStage {
	None = 0,

	BeginOfPipe = uenum(vk::PipelineStageFlagBits::eTopOfPipe),
	EndOfPipe = uenum(vk::PipelineStageFlagBits::eBottomOfPipe),

	TransferBit = uenum(vk::PipelineStageFlagBits::eTransfer),
	HostBit = uenum(vk::PipelineStageFlagBits::eHost),
	VertexInputBit = uenum(vk::PipelineStageFlagBits::eVertexInput),
	VertexBit = uenum(vk::PipelineStageFlagBits::eVertexShader),
	FragmentBit = uenum(vk::PipelineStageFlagBits::eFragmentShader),
	ComputeBit = uenum(vk::PipelineStageFlagBits::eComputeShader),

	// early is load, late is store
	DepthAttachmentOutBit = uenum(vk::PipelineStageFlagBits::eEarlyFragmentTests) | uenum(vk::PipelineStageFlagBits::eLateFragmentTests),
	// counts for both loads and stores
	ColorAttachmentOutBit = uenum(vk::PipelineStageFlagBits::eColorAttachmentOutput),

	AllAttachmentOutBit = DepthAttachmentOutBit | ColorAttachmentOutBit,
	AllShadersBit = VertexBit | FragmentBit | ComputeBit,

	All = uenum(vk::PipelineStageFlagBits::eAllCommands)

};

constexpr PipelineStage operator|(PipelineStage l, PipelineStage r) {
	return PipelineStage(uenum(l) | uenum(r));
}

constexpr PipelineStage operator&(PipelineStage l, PipelineStage r)  {
	return PipelineStage(uenum(l) & uenum(r));
}

constexpr PipelineStage operator~(PipelineStage l)  {
	return PipelineStage(~uenum(l));
}

constexpr bool is_shader_stage(PipelineStage stage) {
	return (stage & PipelineStage::AllShadersBit) != PipelineStage::None;
}

}

#endif // YAVE_GRAPHICS_BARRIERS_PIPELINESTAGE_H
