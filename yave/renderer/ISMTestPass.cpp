/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "ISMTestPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/descriptors/Descriptor.h>
#include <yave/graphics/device/MeshAllocator.h>

namespace yave {

ISMTestPass ISMTestPass::create(FrameGraph& framegraph, const ProbeGenerationPass& probes) {
    const math::Vec2ui ism_size(32);
    const math::Vec2ui ism_count = probes.probe_count;

    FrameGraphMutableImageId ism;

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("ISM clear pass");

        ism = builder.declare_image(VK_FORMAT_R32_UINT, ism_size * ism_count);

        builder.add_color_output(ism);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            recorder.bind_framebuffer(self->framebuffer());
        });
    }

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("ISM test pass");

        builder.add_storage_output(ism);
        builder.add_storage_input(probes.probe_buffer);
        builder.add_external_input(SubBuffer<BufferUsage::StorageBit>(mesh_allocator().position_buffer()));
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = device_resources()[DeviceResources::GenerateISMProgram];
            recorder.dispatch_size(program, math::Vec3ui(ism_count, 1024 * 4), {self->descriptor_sets()[0]});
        });
    }

#if 0
    {
        FrameGraphPassBuilder builder = framegraph.add_pass("ISM splatting pass");

        ism = builder.declare_image(VK_FORMAT_D32_SFLOAT, ism_size * ism_count);

        builder.add_depth_output(ism);
        builder.add_external_input(SubBuffer<BufferUsage::StorageBit>(mesh_allocator().position_buffer()));
        builder.add_inline_input(InlineDescriptor(ism_count));
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            const MaterialTemplate* material = device_resources()[DeviceResources::ISMSplatMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets()[0]);
            render_pass.draw_array(1024, ism_count.x() * ism_count.y());
        });
    }
#endif

    ISMTestPass pass;
    pass.ism = ism;
    return pass;
}

}

