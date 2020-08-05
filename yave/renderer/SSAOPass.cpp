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

#include "SSAOPass.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

struct MiniAOParams {
    float inv_thickness[12];
    float weights[12];
};

static MiniAOParams compute_ao_params(const GBufferPass& gbuffer, const math::Vec2ui& size) {
    constexpr float sample_thickness[12] = {
        std::sqrt(1.0f - 0.2f * 0.2f),
        std::sqrt(1.0f - 0.4f * 0.4f),
        std::sqrt(1.0f - 0.6f * 0.6f),
        std::sqrt(1.0f - 0.8f * 0.8f),
        std::sqrt(1.0f - 0.2f * 0.2f - 0.2f * 0.2f),
        std::sqrt(1.0f - 0.2f * 0.2f - 0.4f * 0.4f),
        std::sqrt(1.0f - 0.2f * 0.2f - 0.6f * 0.6f),
        std::sqrt(1.0f - 0.2f * 0.2f - 0.8f * 0.8f),
        std::sqrt(1.0f - 0.4f * 0.4f - 0.4f * 0.4f),
        std::sqrt(1.0f - 0.4f * 0.4f - 0.6f * 0.6f),
        std::sqrt(1.0f - 0.4f * 0.4f - 0.8f * 0.8f),
        std::sqrt(1.0f - 0.6f * 0.6f - 0.6f * 0.6f)
    };


    const Camera& camera = gbuffer.scene_pass.scene_view.camera();
    const float tan_half_fov = 1.0f / camera.proj_matrix()[0][0];

    const float screenspace_diameter = 10.0f;
    Y_TODO(Do we need to multiply by 2 here?)
    const float thickness_multiplier = /*2.0f **/ 2.0f * tan_half_fov * screenspace_diameter / size.x();
    const float inv_range = 1.0f / thickness_multiplier;

    MiniAOParams params = {};

    for(usize i = 0; i != 12; ++i) {
        params.inv_thickness[i] = inv_range / sample_thickness[i];
    }

    params.weights[0]  = 4.0f * sample_thickness[0];     // Axial
    params.weights[1]  = 4.0f * sample_thickness[1];     // Axial
    params.weights[2]  = 4.0f * sample_thickness[2];     // Axial
    params.weights[3]  = 4.0f * sample_thickness[3];     // Axial
    params.weights[4]  = 4.0f * sample_thickness[4];     // Diagonal
    params.weights[5]  = 8.0f * sample_thickness[5];     // L-shaped
    params.weights[6]  = 8.0f * sample_thickness[6];     // L-shaped
    params.weights[7]  = 8.0f * sample_thickness[7];     // L-shaped
    params.weights[8]  = 4.0f * sample_thickness[8];     // Diagonal
    params.weights[9]  = 8.0f * sample_thickness[9];     // L-shaped
    params.weights[10] = 8.0f * sample_thickness[10];    // L-shaped
    params.weights[11] = 4.0f * sample_thickness[11];    // Diagonal

    // SAMPLE_EXHAUSTIVELY ?
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/72b1ff9830286950960116264b414ea3c842ff47/MiniEngine/Core/SSAO.cpp#L202
    {
        params.weights[0] = 0.0f;
        params.weights[2] = 0.0f;
        params.weights[5] = 0.0f;
        params.weights[7] = 0.0f;
        params.weights[9] = 0.0f;
    }

    float total = 0.0f;
    for(usize i = 0; i != 12; ++i) {
        total += params.weights[i];
    }

    for(usize i = 0; i != 12; ++i) {
        params.weights[i] /= total;
    }

    return params;
}

SSAOPass SSAOPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const SSAOSettings& /*settings*/) {
    Y_TODO(Clamp depth reads to avoid leaks)
    static constexpr ImageFormat ao_format = VK_FORMAT_R8_UNORM;
    static constexpr ImageFormat depth_format = VK_FORMAT_R32_SFLOAT;

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui ao_size = size / 2;

    FrameGraphPassBuilder lindepth_builder = framegraph.add_pass("Depth downsampling pass");

    const auto linear_depth = lindepth_builder.declare_image(depth_format, ao_size);

    lindepth_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
    lindepth_builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
    lindepth_builder.add_storage_output(linear_depth, 0, PipelineStage::ComputeBit);
    lindepth_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources(recorder.device())[DeviceResources::DepthLinearizeDownSampleProgram];
        recorder.dispatch_size(program, ao_size, {self->descriptor_sets()[0]});
    });

    const MiniAOParams params = compute_ao_params(gbuffer, size);

    FrameGraphPassBuilder ssao_builder = framegraph.add_pass("SSAO pass");

    const auto ao = ssao_builder.declare_image(ao_format, ao_size);
    const auto params_buffer = ssao_builder.declare_typed_buffer<MiniAOParams>(1);

    ssao_builder.add_uniform_input(linear_depth, 0, PipelineStage::ComputeBit);
    ssao_builder.add_uniform_input(params_buffer, 0, PipelineStage::ComputeBit);
    ssao_builder.add_storage_output(ao, 0, PipelineStage::ComputeBit);
    ssao_builder.map_update(params_buffer);
    ssao_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        self->resources().mapped_buffer(params_buffer)[0] = params;
        const auto& program = device_resources(recorder.device())[DeviceResources::SSAOProgram];
        recorder.dispatch_size(program, ao_size, {self->descriptor_sets()[0]});
    });

#if 0
    FrameGraphPassBuilder upsample_builder = framegraph.add_pass("SSAO upsampling pass");

    const auto upsampled = upsample_builder.declare_image(depth_format, size);

    upsample_builder.add_uniform_input(ao, 0, PipelineStage::ComputeBit);
    upsample_builder.add_uniform_input(linear_depth, 0, PipelineStage::ComputeBit);
    upsample_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
    upsample_builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
    upsample_builder.add_storage_output(upsampled, 0, PipelineStage::ComputeBit);
    upsample_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources(recorder.device())[DeviceResources::SSAOUpsampleProgram];
        recorder.dispatch_size(program, half_size, {self->descriptor_sets()[0]});
    });
#endif


    SSAOPass pass;
    pass.linear_depth = linear_depth;
    pass.ao = ao;
    return pass;
}

}

