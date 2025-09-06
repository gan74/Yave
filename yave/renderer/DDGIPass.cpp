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

#include "DDGIPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/utils/DebugValues.h>

#include <y/math/random.h>


namespace yave {

static constexpr u32 ddgi_grid_size = 32;
static constexpr u32 ddgi_probe_count = ddgi_grid_size * ddgi_grid_size * ddgi_grid_size;
static constexpr u32 ddgi_probe_rays = 128;

static constexpr u32 ddgi_atlas_size = 256;
static constexpr u32 ddgi_probe_size_no_border = 14;
static constexpr u32 ddgi_probe_size_border = ddgi_probe_size_no_border + 2;


static auto generate_random_tbn(u64 seed) {
    math::FastRandom rng(seed);
    auto random = [&] { return float(double(rng()) / double(rng.max())) * 2.0f - 1.0f; };
    const math::Vec3 normal = math::Vec3(random(), random(), random()).normalized();

    math::Vec3 tangent;
    for(;;) {
        tangent = math::Vec3(random(), random(), random()).normalized();
        if(std::abs(tangent.dot(normal)) < 0.9f) {
            break;
        }
    }

    const math::Vec3 bitangent = normal.cross(tangent).normalized();
    tangent = normal.cross(bitangent);

    const std::array<math::Vec4, 3> tbn {
        math::Vec4(normal, 0.0f),  math::Vec4(tangent, 0.0f),  math::Vec4(bitangent, 0.0f)
    };

    return tbn;
}

static std::tuple<FrameGraphTypedBufferId<shader::DDGIRayHit>, FrameGraphMutableImageId, FrameGraphMutableImageId> trace_probes(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const auto [ibl_probe, intensity, _] = visibility.ibl_probe();

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe tracing pass");

    const auto hits = builder.declare_typed_buffer<shader::DDGIRayHit>(ddgi_probe_count * ddgi_probe_rays);
    const auto directionals = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());

    const auto irradiance = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, math::Vec2ui(ddgi_probe_size_border * ddgi_atlas_size));
    const auto distance = builder.declare_image(VK_FORMAT_R16G16_SFLOAT, math::Vec2ui(ddgi_probe_size_border * ddgi_atlas_size));

    if(editor::debug_values().value<bool>("Clear probes")) {
        builder.clear_before_pass(irradiance);
        builder.clear_before_pass(distance);
    }

    builder.map_buffer(directionals);

    builder.add_storage_output(hits);
    builder.add_external_input(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_storage_input(directionals);
    builder.add_external_input(*ibl_probe);
    builder.add_uniform_input(irradiance);
    builder.add_uniform_input(distance);
    builder.add_inline_input(std::pair{generate_random_tbn(framegraph.frame_id()), intensity});
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto mapping = self->resources().map_buffer(directionals);
        for(usize i = 0; i != visibility.directional_lights.size(); ++i) {
            const DirectionalLightComponent& light = visibility.directional_lights[i]->component;
            mapping[i] = {
                -light.direction().normalized(),
                std::cos(light.disk_size()),
                light.color() * light.intensity(),
                u32(light.cast_shadow() ? 1 : 0), {}
            };
        }

        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };

        const auto& program = device_resources()[DeviceResources::TraceProbesProgram];
        recorder.dispatch_threads(program, math::Vec2ui(ddgi_probe_count * ddgi_probe_rays, 1), desc_sets);
    });

    return {hits, irradiance, distance};
}

static void update_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphTypedBufferId<shader::DDGIRayHit> hits, FrameGraphMutableImageId irradiance, FrameGraphMutableImageId distance) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Update probes pass");

    builder.add_storage_output(irradiance);
    builder.add_storage_output(distance);
    builder.add_storage_input(hits);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_inline_input(editor::debug_values().value<float>("Depth sharpness", 2.0f));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::UpdateProbesProgram];
        recorder.dispatch_threads(program, math::Vec3ui(ddgi_probe_size_border, ddgi_probe_size_border, ddgi_probe_count), self->descriptor_set());
    });
}

static FrameGraphImageId apply_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId irradiance, FrameGraphImageId distance) {
    const math::Vec2 size = framegraph.image_size(gbuffer.depth);

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Apply probes pass");

    const auto gi = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, size);

    builder.add_storage_output(gi);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(irradiance);
    builder.add_uniform_input(distance);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(tlas);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::ApplyProbesProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    return gi;
}


[[maybe_unused]]
static FrameGraphImageId debug_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphImageId irradiance, FrameGraphImageId distance) {
    FrameGraphPassBuilder builder = framegraph.add_pass("Debug probes pass");

    const auto depth = builder.declare_copy(gbuffer.depth);
    const auto color = builder.declare_copy(lit);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_uniform_input(irradiance);
    builder.add_uniform_input(distance);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::DebugProbesTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());

        const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
        render_pass.draw(sphere.draw_data(), u32(ddgi_probe_count));
    });

    return color;
}

[[maybe_unused]]
static FrameGraphImageId debug_hits(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphTypedBufferId<shader::DDGIRayHit> hits) {

    FrameGraphPassBuilder builder = framegraph.add_pass("Debug hits pass");

    const auto depth = builder.declare_copy(gbuffer.depth);
    // const auto color = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, framegraph.image_size(lit));
    const auto color = builder.declare_copy(lit);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_storage_input(hits);
    builder.add_uniform_input(gbuffer.scene_pass.camera);

    math::Vec3i coord;
    coord.x() = editor::debug_values().value<int>("Debug probe X");
    coord.y() = editor::debug_values().value<int>("Debug probe Y");
    coord.z() = editor::debug_values().value<int>("Debug probe Z");
    builder.add_inline_input(coord);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::DebugHitsTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());
        render_pass.draw_array(ddgi_probe_count * ddgi_probe_rays * 2);
    });

    return color;
}


DDGIPass DDGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    if(!raytracing_enabled()) {
        return {lit};
    }

    const auto region = framegraph.region("GI");

    const auto [hits, irradiance, distance] = trace_probes(framegraph, gbuffer);
    update_probes(framegraph, gbuffer, hits, irradiance, distance);
    const FrameGraphImageId gi = apply_probes(framegraph, gbuffer, irradiance, distance);

    DDGIPass pass;
    pass.irradiance = irradiance;
    pass.distance = distance;
    pass.gi = gi;

    if(editor::debug_values().value<bool>("Show DDGI probes")) {
        pass.gi = debug_probes(framegraph, gbuffer, pass.gi, irradiance, distance);
    }

    if(editor::debug_values().value<bool>("Show DDGI hits")) {
        pass.gi = debug_hits(framegraph, gbuffer, pass.gi, hits);
    }

    return pass;
}

}

