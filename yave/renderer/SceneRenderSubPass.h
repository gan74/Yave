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
#ifndef YAVE_RENDERER_SCENERENDERSUBPASS_H
#define YAVE_RENDERER_SCENERENDERSUBPASS_H

#include <yave/scene/SceneView.h>
#include <yave/framegraph/FrameGraphResourceId.h>
#include <yave/systems/RendererSystem.h>

#include "SceneVisibilitySubPass.h"

namespace yave {

struct SceneRenderSubPass {
    SceneView scene_view;

    i32 main_descriptor_set_index = -1;
    FrameGraphTypedBufferId<uniform::Camera> camera;

    std::shared_ptr<core::Vector<ecs::EntityId>> visible;
    RendererSystem::RenderFunc render_func;

    static SceneRenderSubPass create(FrameGraphPassBuilder& builder, const SceneView& scene_view, const SceneVisibilitySubPass& visibility, PassType pass_type);
    static SceneRenderSubPass create(FrameGraphPassBuilder& builder, const CameraBufferPass& camera, const SceneVisibilitySubPass& visibility, PassType pass_type);

    void render(RenderPassRecorder& render_pass, const FrameGraphPass* pass) const;
};


}

#endif // YAVE_RENDERER_SCENERENDERSUBPASS_H

