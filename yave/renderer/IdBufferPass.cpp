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

#include "IdBufferPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

IdBufferPass IdBufferPass::create(FrameGraph& framegraph, const CameraBufferPass& camera, const SceneVisibilitySubPass& visibility, const math::Vec2ui& size) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat id_format = VK_FORMAT_R32_UINT;

    FrameGraphPassBuilder builder = framegraph.add_pass("Id-buffer pass");

    const auto depth = builder.declare_image(depth_format, size);
    const auto id = builder.declare_image(id_format, size);

    IdBufferPass pass;
    pass.depth = depth;
    pass.id = id;
    pass.scene_pass = SceneRenderSubPass::create(builder, camera, visibility, PassType::Id);

    builder.add_depth_output(depth);
    builder.add_color_output(id);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        pass.scene_pass.render(render_pass, self);
    });

    return pass;
}

}

