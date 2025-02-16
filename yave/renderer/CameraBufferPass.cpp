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

#include "CameraBufferPass.h"

#include <yave/camera/Camera.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

static math::Vec2 compute_jitter(JitterSettings::JitterSeq jitter, u64 jitter_index) {
    switch(jitter) {
        case JitterSettings::JitterSeq::Weyl:
            return math::Vec2(math::weyl_2d<double>(jitter_index));

        case JitterSettings::JitterSeq::R2:
            return math::Vec2(math::golden_r2_2d<double>(jitter_index));

        default:
            y_fatal("Unknown jitter type");
    }

    return math::Vec2(0.5f);
}


CameraBufferPass CameraBufferPass::create_no_jitter(FrameGraph& framegraph, const SceneView& view, FrameGraphPersistentResourceId persistent_id) {
    JitterSettings settings;
    {
        settings.jitter_intensity = 0.0f;
    }

    return create(framegraph, view, {}, persistent_id, settings);
}

CameraBufferPass CameraBufferPass::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, FrameGraphPersistentResourceId persistent_id, const JitterSettings& settings) {
    const bool enabled = settings.jitter_intensity > 0.0f;

    const u64 jitter_index = framegraph.frame_id() % 1024;
    const Camera jittered_camera = enabled ? view.camera().jittered(compute_jitter(settings.jitter, jitter_index), size, settings.jitter_intensity) : view.camera();
    const SceneView jittered_view = SceneView(view.scene(), jittered_camera);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Camera buffer pass");

    const auto camera = builder.declare_typed_buffer<shader::Camera>();
    builder.map_buffer(camera, shader::Camera(jittered_view.camera()));

    builder.add_input_usage(camera, BufferUsage::UniformBit);

    const FrameGraphBufferId prev_camera = framegraph.make_persistent_and_get_prev(camera, persistent_id);

    if(prev_camera.is_valid()) {
        builder.add_storage_output(camera);
        builder.add_uniform_input(prev_camera);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = device_resources()[DeviceResources::PrevCameraProgram];
            recorder.dispatch(program, math::Vec3ui(1u), self->descriptor_sets());
        });
    }

    CameraBufferPass pass;
    pass.view = jittered_view;
    pass.unjittered_view = view;
    pass.camera = camera;
    return pass;
}

}

