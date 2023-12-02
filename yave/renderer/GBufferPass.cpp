/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "GBufferPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

GBufferPass GBufferPass::create(FrameGraph& framegraph, const CameraBufferPass& camera, const math::Vec2ui& size) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat motion_format = VK_FORMAT_R16G16_SFLOAT;
    static constexpr ImageFormat color_format = VK_FORMAT_R8G8B8A8_SRGB;
    static constexpr ImageFormat normal_format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    static constexpr ImageFormat emissive_format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;

    FrameGraphPassBuilder builder = framegraph.add_pass("G-buffer pass");

    const auto depth = builder.declare_image(depth_format, size);
    const auto motion = builder.declare_image(motion_format, size);
    const auto color = builder.declare_image(color_format, size);
    const auto normal = builder.declare_image(normal_format, size);
    const auto emissive = builder.declare_image(emissive_format, size);

    GBufferPass pass;
    pass.depth = depth;
    pass.motion = motion;
    pass.color = color;
    pass.normal = normal;
    pass.emissive = emissive;
    pass.scene_pass = SceneRenderSubPass::create(builder, camera);

    builder.add_depth_output(depth);
    builder.add_color_output(motion);
    builder.add_color_output(color);
    builder.add_color_output(normal);
    builder.add_color_output(emissive);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        pass.scene_pass.render(render_pass, self);
    });

    return pass;
}

}

