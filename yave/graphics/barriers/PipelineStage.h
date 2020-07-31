/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
enum class PipelineStage : u32{
    None = 0,

    BeginOfPipe     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    EndOfPipe       = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,

    TransferBit     = VK_PIPELINE_STAGE_TRANSFER_BIT,
    HostBit         = VK_PIPELINE_STAGE_HOST_BIT,
    VertexInputBit  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    VertexBit       = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    FragmentBit     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    ComputeBit      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,

    // early is load, late is store
    DepthAttachmentOutBit   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    // counts for both loads and stores
    ColorAttachmentOutBit   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

    AllAttachmentOutBit     = DepthAttachmentOutBit | ColorAttachmentOutBit,
    AllShadersBit           = VertexBit | FragmentBit | ComputeBit,

    All = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT

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

