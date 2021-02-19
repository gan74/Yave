/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <yave/scene/Renderable.h>

namespace yave {

class RenderPassRecorder;
class FrameGraphPassBuilder;

struct SceneRenderSubPass {
    static constexpr usize max_batch_size = 128 * 1024;

    SceneView scene_view;
    usize descriptor_set_index = 0;

    Y_TODO(remove mutable)
    FrameGraphMutableTypedBufferId<Renderable::CameraData> camera_buffer;
    FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer;

    static SceneRenderSubPass create(FrameGraphPassBuilder& builder, const SceneView& view);
    void render(RenderPassRecorder& recorder, const FrameGraphPass* pass) const;

};


}

#endif // YAVE_RENDERER_SCENERENDERSUBPASS_H

