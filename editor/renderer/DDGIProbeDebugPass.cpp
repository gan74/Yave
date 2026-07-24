/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "DDGIProbeDebugPass.h"

#include <editor/EditorResources.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/graphics.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/assets/AssetPtr.h>

namespace editor {

// Must match shaders/lib/ddgi.slang
static constexpr u32 ddgi_grid_size = 32;
static constexpr u32 ddgi_probe_count = ddgi_grid_size * ddgi_grid_size * ddgi_grid_size;

DDGIProbeDebugPass DDGIProbeDebugPass::create(FrameGraph& framegraph,
                                              FrameGraphTypedBufferId<shader::Camera> camera,
                                              FrameGraphImageId in_color,
                                              FrameGraphImageId in_depth,
                                              const yave::DDGIPass& ddgi,
                                              float probe_radius) {
    if(!ddgi.is_valid()) {
        return {in_color, in_depth};
    }

    FrameGraphPassBuilder builder = framegraph.add_pass("DDGI probe debug pass");

    const auto color = builder.declare_copy(in_color);
    const auto depth = builder.declare_copy(in_depth);

    const AssetPtr<StaticMesh> sphere = device_resources()[DeviceResources::SimpleSphereMesh];

    const struct Params {
        float probe_spacing;
        float probe_radius;
        u32 mesh_data_index;
        u32 _pad;
    } params {
        ddgi.probe_spacing,
        probe_radius * ddgi.probe_spacing,
        sphere->mesh_data_index(),
        0,
    };

    builder.add_color_output(color);
    builder.add_depth_output(depth);

    builder.add_uniform_input(camera);
    builder.add_external_input(Descriptor(mesh_allocator().mesh_data_buffer()));
    builder.add_uniform_input(ddgi.irradiance, SamplerType::LinearClamp);
    builder.add_inline_input(params);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const MaterialTemplate* material = resources()[EditorResources::DDGIProbeDebugMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());
        render_pass.draw(sphere->draw_data(), ddgi_probe_count);
    });

    return {color, depth};
}

}
