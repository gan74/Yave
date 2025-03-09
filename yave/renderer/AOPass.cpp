/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "AOPass.h"
#include "DownsamplePass.h"
#include "TAAPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/DeviceResources.h>

#include <y/math/random.h>

namespace yave {

struct MiniAOParams {
    float inv_thickness[12];
    float weights[12];
};

struct UpsampleParams {
    float step_size;
    float noise_weight;
    float blur_tolerance;
    float upsample_tolerance;
};

static float compute_tan_half_fov(const GBufferPass& gbuffer) {
    const Camera& camera = gbuffer.scene_pass.scene_view.camera();
    return 1.0f / camera.proj_matrix()[0][0];
}

static MiniAOParams compute_ao_params(float tan_half_fov, usize screen_size_x) {
    const float sample_thickness[12] = {
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



    const float screenspace_diameter = 10.0f;
    Y_TODO(Do we need to multiply by 2 here?)
    const float thickness_multiplier = /* 2.0f * */ 2.0f * tan_half_fov * screenspace_diameter / screen_size_x;
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

static FrameGraphImageId compute_linear_depth(FrameGraph& framegraph, const GBufferPass& gbuffer, const math::Vec2ui& size) {
    static constexpr ImageFormat depth_format = VK_FORMAT_R32_SFLOAT;

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Depth linearization pass");

    const auto linear_depth = builder.declare_image(depth_format, size);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_output(linear_depth);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::LinearizeDepthProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    return linear_depth;
}

static FrameGraphMutableImageId upsample_mini_ao(FrameGraph& framegraph,
                                     u32 final_size_x,
                                     const math::Vec2ui& output_size,
                                     const AOSettings& settings,
                                     FrameGraphImageId hi_depth,
                                     FrameGraphImageId lo_depth,
                                     FrameGraphImageId hi_ao,
                                     FrameGraphImageId lo_ao = FrameGraphImageId()) {

    const bool merge = lo_ao.is_valid();
    const ImageFormat format = framegraph.image_format(hi_ao);
    //const math::Vec2ui hi_size = framegraph.image_size(hi_depth);
    const math::Vec2ui lo_size = framegraph.image_size(lo_depth);

    const float step_size = final_size_x / float(lo_size.x());
    float blur_tolerance = 1.0f - std::pow(10.0f, -settings.mini_engine.blur_tolerance) * step_size;
    blur_tolerance *= blur_tolerance;
    const float upsample_tolerance = std::pow(10.0f, -settings.mini_engine.upsample_tolerance);
    const float noise_filter_weight = 1.0f / (std::pow(10.0f, -settings.mini_engine.noise_filter_tolerance) + upsample_tolerance);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("SSAO upsample pass");

    const auto upsampled = builder.declare_image(format, output_size);

    const Texture& white = *device_resources()[DeviceResources::WhiteTexture];
    builder.add_uniform_input_with_default(lo_ao, Descriptor(white));
    builder.add_uniform_input(hi_ao);
    builder.add_uniform_input(lo_depth);
    builder.add_uniform_input(hi_depth);
    builder.add_storage_output(upsampled);
    builder.add_inline_input(UpsampleParams{step_size, noise_filter_weight, blur_tolerance, upsample_tolerance});
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[merge ? DeviceResources::SSAOUpsampleMergeProgram : DeviceResources::SSAOUpsampleProgram];
        recorder.dispatch_threads(program, lo_size + math::Vec2ui(2), self->descriptor_set());
    });

    return upsampled;
}

static FrameGraphImageId compute_mini_ao(FrameGraph& framegraph, FrameGraphImageId linear_depth, float tan_half_fov) {
    const math::Vec2ui size = framegraph.image_size(linear_depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("SSAO pass");

    const auto ao = builder.declare_image(VK_FORMAT_R8_UNORM, size);

    builder.add_uniform_input(linear_depth);
    builder.add_storage_output(ao);
    builder.add_inline_input(compute_ao_params(tan_half_fov, size.x()));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::SSAOProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    return ao;
}







static auto generate_sample_dirs(u64 seed) {
    y_profile();

    std::array<math::Vec4, 256> dirs;
    const float golden = (1.0f + std::sqrt(5.0f)) * 0.5f;
    for(usize i = 0; i != dirs.size(); ++i) {
        const float wrapped_index = float((i + usize(seed)) % dirs.size());

        const float theta = 2.0f * math::pi<float> * wrapped_index * golden;
        const float phi = std::acos(1.0f - 2.0f * wrapped_index / float(dirs.size()));
        dirs[i] = math::Vec4(math::Vec2(std::cos(theta), std::sin(theta)) * std::sin(phi), std::cos(phi), 0.0f);
    }

    std::shuffle(dirs.begin(), dirs.end(), math::FastRandom(seed));

    return dirs;
}

static FrameGraphImageId compute_rtao(FrameGraph& framegraph, const GBufferPass& gbuffer, u32 ray_count, float max_dist) {
    const auto sample_dirs = generate_sample_dirs(framegraph.frame_id());

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTAO pass");

    const auto ao = builder.declare_image(VK_FORMAT_R8_UNORM, size);
    const auto sample_dir_buffer = builder.declare_typed_buffer<std::remove_cvref_t<decltype(sample_dirs)>>();

    builder.map_buffer(sample_dir_buffer, sample_dirs);

    const TLAS& tlas = gbuffer.scene_pass.scene_view.scene()->tlas();

    builder.add_storage_output(ao);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(sample_dir_buffer);
    builder.add_inline_input(std::pair<u32, float>{ray_count, max_dist});
    builder.add_descriptor_binding(Descriptor(tlas));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::RTAOProgram], size, self->descriptor_set());
    });

    return ao;
}


static FrameGraphImageId filter_ao(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_ao, bool vertical, float sigma) {
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass(vertical ? "Filter RTAO vertical pass" : "Filter RTAO horizontal pass");

    const auto filtered = builder.declare_image(VK_FORMAT_R8_UNORM, size);

    const auto weights = math::compute_gaussian_weights<float, 4>(sigma);

    struct Params {
        std::array<float, 4> weights;
        math::Vec2i offset;
    };

    builder.add_uniform_input(in_ao);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_output(filtered);
    builder.add_inline_input(Params{weights, vertical ? math::Vec2i(0, 1) : math::Vec2i(1, 0)});
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::FilterAOProgram], size, self->descriptor_set());
    });

    return filtered;
}




AOPass AOPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const TemporalPrePass& temporal, const AOSettings& settings) {
    const auto region = framegraph.region("AO");

    const AOSettings::AOMethod method = (settings.method == AOSettings::AOMethod::RTAO && !raytracing_enabled())
        ? AOSettings::AOMethod::RTAOFallback
        : settings.method;

    FrameGraphImageId ao;
    switch(method) {
        case AOSettings::AOMethod::MiniEngine: {
            y_always_assert(settings.mini_engine.level_count > 1, "AOSettings::level_count needs to be at least 2");
            const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
            const FrameGraphImageId linear_depth = compute_linear_depth(framegraph, gbuffer, size);
            const DownsamplePass downsample = DownsamplePass::create(framegraph, linear_depth, settings.mini_engine.level_count, DownsamplePass::Filter::Average, true);
            const float tan_half_fov = compute_tan_half_fov(gbuffer);
            for(usize i = downsample.mips.size() - 1; i > 0; --i) {
                const math::Vec2ui output_size = framegraph.image_size(downsample.mips[i - 1]);
                const FrameGraphImageId hi = compute_mini_ao(framegraph, downsample.mips[i], tan_half_fov);
                ao = upsample_mini_ao(framegraph, size.x(), output_size, settings, downsample.mips[i - 1], downsample.mips[i], hi, ao);
            }
        } break;

        case AOSettings::AOMethod::RTAO: {
            if(raytracing_enabled()) {
                ao = compute_rtao(framegraph, gbuffer, settings.rtao.ray_count, settings.rtao.max_dist);

                if(settings.rtao.temporal) {
                    static const FrameGraphPersistentResourceId persistent_id = FrameGraphPersistentResourceId::create();
                    ao = TAAPass::create_simple(framegraph, temporal, ao, persistent_id).anti_aliased;
                }

                if(settings.rtao.filter_sigma > 0.0f) {
                    ao = filter_ao(framegraph, gbuffer, ao, false, settings.rtao.filter_sigma);
                    ao = filter_ao(framegraph, gbuffer, ao, true, settings.rtao.filter_sigma);
                }
            }
        } break;

        default:
        break;
    }

    AOPass pass;
    pass.ao = ao;
    return pass;
}

}

