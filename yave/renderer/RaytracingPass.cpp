/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "RaytracingPass.h"
#include "CameraBufferPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

RaytracingPass RaytracingPass::create(FrameGraph& framegraph, const CameraBufferPass& camera, const math::Vec2ui& size) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Raaytracing");

    const auto raytraced = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, size);

    const auto tlas = camera.view.scene()->create_tlas();

    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_uniform_input(camera.camera);
    builder.add_storage_output(raytraced);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.raytrace(device_resources()[DeviceResources::BasicRaytracingProgram], size, self->descriptor_sets());
    });

    RaytracingPass pass;
    pass.raytraced = raytraced;
    return pass;
}


}

