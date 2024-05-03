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

#include "SceneRenderSubPass.h"
#include "TAAPass.h"

#include <yave/scene/EcsScene.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/ecs/EntityWorld.h>


namespace yave {

static void fill_scene_render_pass(SceneRenderSubPass& pass, FrameGraphPassBuilder& builder, PassType pass_type) {
    pass.render_func = pass.scene_view.scene()->prepare_render(builder, pass.scene_view.camera(), pass_type);

    pass.main_descriptor_set_index = builder.next_descriptor_set_index();
    builder.add_uniform_input(pass.camera, PipelineStage::None, pass.main_descriptor_set_index);
}


SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& scene_view, const SceneVisibilitySubPass& visibility, PassType pass_type) {
    const auto camera = builder.declare_typed_buffer<shader::Camera>();
    builder.map_buffer(camera, shader::Camera(scene_view.camera()));

    SceneRenderSubPass pass;
    pass.scene_view = scene_view;
    pass.camera = camera;
    pass.visibility = visibility;

    fill_scene_render_pass(pass, builder, pass_type);

    return pass;
}

SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const CameraBufferPass& camera, const SceneVisibilitySubPass& visibility, PassType pass_type) {
    SceneRenderSubPass pass;
    pass.scene_view = camera.view;
    pass.camera = camera.camera;
    pass.visibility = visibility;

    fill_scene_render_pass(pass, builder, pass_type);

    return pass;
}

void SceneRenderSubPass::render(RenderPassRecorder& render_pass, const FrameGraphPass* pass) const {
    render_pass.set_main_descriptor_set(pass->descriptor_sets()[main_descriptor_set_index]);

    if(render_func) {
        render_func(render_pass, pass);
    }
}

}

